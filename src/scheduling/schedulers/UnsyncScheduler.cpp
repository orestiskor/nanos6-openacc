/*
	This file is part of Nanos6 and is licensed under the terms contained in the COPYING file.

	Copyright (C) 2019-2020 Barcelona Supercomputing Center (BSC)
*/

#include "UnsyncScheduler.hpp"
#include "dependencies/DataTrackingSupport.hpp"
#include "executors/threads/CPUManager.hpp"

#if USE_DEVMEMMAN
#include "hardware/device/DeviceMemManager.hpp"
#endif


UnsyncScheduler::UnsyncScheduler(
	SchedulingPolicy,
	bool enablePriority,
	bool enableImmediateSuccessor
) :
	_queues(nullptr),
	_numQueues(0),
	_roundRobinQueues(0),
	_deadlineTasks(nullptr),
	_enableImmediateSuccessor(enableImmediateSuccessor),
	_enablePriority(enablePriority)
{
	if (enableImmediateSuccessor) {
		_immediateSuccessorTasks = immediate_successor_tasks_t(CPUManager::getTotalCPUs(), nullptr);
	}
}

UnsyncScheduler::~UnsyncScheduler()
{
	assert(_numQueues > 0);

	for (uint64_t i = 0; i < _numQueues; i++) {
		if (_queues[i] != nullptr) {
			delete _queues[i];
		}
	}

	MemoryAllocator::free(_queues, _numQueues * sizeof(ReadyQueue *));
}

void UnsyncScheduler::regularAddReadyTask(Task *task, bool unblocked)
{
	uint64_t NUMAid = 0;

	if (task->getDeviceType() == nanos6_host_device) {
		NUMAid = task->getNUMAHint();

		// In case there is no hint, use round robin to balance the load
		if (NUMAid == (uint64_t) -1) {
			do {
				NUMAid = _roundRobinQueues;
				_roundRobinQueues = (_roundRobinQueues + 1) % _numQueues;
			} while (_queues[NUMAid] == nullptr);
		}
	}
#if USE_DEVMEMMAN // Using any device in the config will enable this,
				  // if not then deviceType will be nanos6_host_device *every* time
	else {
		NUMAid = DeviceMemManager::computeDeviceAffinity(task);
	}
#endif

	assert(NUMAid < _numQueues);

	assert(_queues[NUMAid] != nullptr);
	_queues[NUMAid]->addReadyTask(task, unblocked);
}

Task *UnsyncScheduler::regularGetReadyTask(ComputePlace *computePlace)
{
	uint64_t NUMAid = 0;
	nanos6_device_t deviceType = computePlace->getType();

	if (_numQueues > 1) {
		if (deviceType == nanos6_host_device) {
			NUMAid = ((CPU *)computePlace)->getNumaNodeId();
		}
		else {
			NUMAid = (uint64_t)computePlace->getIndex();
		}

	}
	assert(NUMAid < _numQueues);

	Task *result = nullptr;
	result = _queues[NUMAid]->getReadyTask(computePlace);
	if (result != nullptr)
		return result;

	if (_numQueues > 1 && deviceType == nanos6_host_device) {
		// Try to steal considering distance and load balance, only for SMP tasks; no stealing for device tasks yet
		const std::vector<uint64_t> &distances = HardwareInfo::getNUMADistances();

		const double distanceThold = DataTrackingSupport::getDistanceThreshold();
		const double loadThold = DataTrackingSupport::getLoadThreshold();

		// Ideally, we want to steal from closer sockets with many tasks, so we
		// will use this score function: score = 100/distance + ready_tasks/5
		// The highest score the better
		uint64_t score = 0;
		uint64_t chosen = (uint64_t) -1;
		for (uint64_t q = 0; q < _numQueues; q++) {
			if (q != NUMAid && _queues[q] != nullptr) {
				size_t numReadyTasks = _queues[q]->getNumReadyTasks();

				if (numReadyTasks > 0) {
					uint64_t distance = distances[q * _numQueues + NUMAid];
					uint64_t loadFactor = numReadyTasks;

					if (distance < distanceThold && loadFactor > loadThold) {
						chosen = q;
						break;
					}

					assert(distance != 0);
					uint64_t tmpscore = 100 / distance + loadFactor / 5;
					if (tmpscore >= score) {
						score = tmpscore;
						chosen = q;
					}
				}
			}
		}

		if (chosen != (uint64_t) -1) {
			result = _queues[chosen]->getReadyTask(computePlace);
			assert(result != nullptr);
		}
	}

	return result;
}
