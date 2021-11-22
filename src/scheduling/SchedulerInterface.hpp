/*
	This file is part of Nanos6 and is licensed under the terms contained in the COPYING file.

	Copyright (C) 2015-2020 Barcelona Supercomputing Center (BSC)
*/

#ifndef SCHEDULER_INTERFACE_HPP
#define SCHEDULER_INTERFACE_HPP

#include "executors/threads/CPUManager.hpp"
#include "hardware/places/ComputePlace.hpp"
#include "scheduling/schedulers/HostScheduler.hpp"
#include "scheduling/schedulers/device/DeviceScheduler.hpp"
#include "tasks/Task.hpp"
#include "tasks/TaskImplementation.hpp"

#include <InstrumentTaskStatus.hpp>

class SchedulerInterface {
	HostScheduler *_hostScheduler;
	DeviceScheduler *_deviceSchedulers[nanos6_device_type_num];

	static ConfigVariable<std::string> _schedulingPolicy;
	static ConfigVariable<bool> _enableImmediateSuccessor;
	static ConfigVariable<bool> _enablePriority;

#ifdef EXTRAE_ENABLED
	std::atomic<Task *> _mainTask;
	bool _mainFirstRunCompleted = false;
#endif

public:
	SchedulerInterface();
	virtual ~SchedulerInterface();

	virtual inline void addReadyTask(Task *task, ComputePlace *computePlace, ReadyTaskHint hint = NO_HINT)
	{
#ifdef EXTRAE_ENABLED
		if (task->isMainTask() && !_mainFirstRunCompleted) {
			Task *expected = nullptr;
			bool exchanged = _mainTask.compare_exchange_strong(expected, task);
			FatalErrorHandler::failIf(!exchanged);
			CPUManager::forcefullyResumeFirstCPU();
			return;
		}
#endif
		nanos6_device_t taskType;
		for (uint8_t i = 0; i < task->getImplementationCount(); i++) {
			taskType = (nanos6_device_t) task->getDeviceType(i);
			assert(taskType != nanos6_cluster_device);
		}

		if (task->getImplementationCount() > 1) {
			// select one implementation type
			// e.g. query Energy-And-Load Manager ==> returns device type ==> send it to that device
			// Initial PoC, assume 2 implements only and just pick one. *DUMMY*
			if (true) {
				if (taskType == nanos6_host_device) {
					_hostScheduler->addReadyTask(task, computePlace, hint);
				} else {
					assert(taskType == _deviceSchedulers[taskType]->getDeviceType());
					_deviceSchedulers[taskType]->addReadyTask(task, computePlace, hint);
				}

			}
		}
		else {
			if (taskType == nanos6_host_device) {
				_hostScheduler->addReadyTask(task, computePlace, hint);
			} else {
				assert(taskType == _deviceSchedulers[taskType]->getDeviceType());
				_deviceSchedulers[taskType]->addReadyTask(task, computePlace, hint);
			}
		}
	}

	virtual inline void addReadyTasks(
		nanos6_device_t taskType,
		Task *tasks[],
		const size_t numTasks,
		ComputePlace *computePlace,
		ReadyTaskHint hint)
	{
		assert(taskType != nanos6_cluster_device);

		if (taskType == nanos6_host_device) {
			_hostScheduler->addReadyTasks(tasks, numTasks, computePlace, hint);
		} else {
			assert(taskType == _deviceSchedulers[taskType]->getDeviceType());
			_deviceSchedulers[taskType]->addReadyTasks(tasks, numTasks, computePlace, hint);
		}
	}

	virtual inline Task *getReadyTask(ComputePlace *computePlace)
	{
		assert(computePlace != nullptr);
		nanos6_device_t computePlaceType = computePlace->getType();

		if (computePlaceType == nanos6_host_device) {
#ifdef EXTRAE_ENABLED
			Task *result = nullptr;
			if (CPUManager::isFirstCPU(computePlace->getIndex())) {
				 while (!_mainFirstRunCompleted) {
					if (_mainTask != nullptr) {
						result = _mainTask;
						bool exchanged = _mainTask.compare_exchange_strong(result, nullptr);
						FatalErrorHandler::failIf(!exchanged);
						_mainFirstRunCompleted = true;
						return result;
					}
				}
			}
#endif
			return _hostScheduler->getReadyTask(computePlace);
		} else {
			assert(computePlaceType != nanos6_cluster_device);
			return _deviceSchedulers[computePlaceType]->getReadyTask(computePlace);
		}
	}

	virtual inline bool isServingTasks() const
	{
		return _hostScheduler->isServingTasks();
	}

	virtual std::string getName() const = 0;

	//! \brief Check whether task priority is considered
	static inline bool isPriorityEnabled()
	{
		return _enablePriority;
	}
};

#endif // SCHEDULER_INTERFACE_HPP
