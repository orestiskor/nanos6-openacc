/*
	This file is part of Nanos6 and is licensed under the terms contained in the COPYING file.

	Copyright (C) 2015-2021 Barcelona Supercomputing Center (BSC)
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <alloca.h>
#include <atomic>
#include <cassert>
#include <cstring>
#include <pthread.h>

#include "CPUManager.hpp"
#include "TaskFinalization.hpp"
#include "TaskFinalizationImplementation.hpp"
#include "ThreadManager.hpp"
#include "WorkerThread.hpp"
#include "dependencies/SymbolTranslation.hpp"
#include "hardware/HardwareInfo.hpp"
#include "scheduling/Scheduler.hpp"
#include "system/If0Task.hpp"
#include "system/TrackingPoints.hpp"
#include "tasks/LoopGenerator.hpp"
#include "tasks/Task.hpp"
#include "tasks/TaskImplementation.hpp"

#include <DataAccessRegistration.hpp>
#include <InstrumentInstrumentationContext.hpp>
#include <InstrumentThreadInstrumentationContext.hpp>
#include <InstrumentWorkerThread.hpp>

void WorkerThread::initialize()
{
	markAsCurrentWorkerThread();

	CPU *cpu = getComputePlace();
	assert(cpu != nullptr);

	Instrument::ThreadInstrumentationContext instrumentationContext(
		Instrument::task_id_t(),
		cpu->getInstrumentationId(),
		getInstrumentationId()
	);

	// Runtime Tracking Point - A thread is initializing
	TrackingPoints::threadInitialized(this, cpu);

	// This is needed for kernel-level threads to stop them after initialization
	synchronizeInitialization();
}


void WorkerThread::body()
{
	initialize();

	CPU *cpu = getComputePlace();
	assert(cpu != nullptr);

	Instrument::ThreadInstrumentationContext instrumentationContext(
		Instrument::task_id_t(), cpu->getInstrumentationId(), _instrumentationId
	);

	// The WorkerThread will iterate until its CPU status signals that there is
	// an ongoing shutdown and thus the thread must stop executing
	while (CPUManager::checkCPUStatusTransitions(this) != CPU::shutdown_status) {
		cpu = getComputePlace();
		assert(cpu != nullptr);

		// Update the CPU since the thread may have migrated
		instrumentationContext.updateComputePlace(cpu->getInstrumentationId());

		// There should not be any pre-assigned task
		assert(_task == nullptr);

		_task = Scheduler::getReadyTask(cpu, this);
		if (_task != nullptr) {
			WorkerThread *assignedThread = _task->getThread();

			// A task already assigned to another thread
			if (assignedThread != nullptr) {
				_task = nullptr;

				ThreadManager::addIdler(this);

				// Runtime Tracking Point - The current thread will suspend
				TrackingPoints::threadWillSuspend(this, cpu);

				switchTo(assignedThread);
			} else {
				Instrument::workerThreadObtainedTask();
				// If the task is a taskfor, the CPUManager may want to unidle
				// collaborators to help execute it
				if (_task->isTaskfor()) {
					CPUManager::executeCPUManagerPolicy(cpu, HANDLE_TASKFOR, 0);
				}

				if (_task->isIf0()) {
					// An if0 task executed outside of the implicit taskwait of its parent (i.e. not inline)
					Task *if0Task = _task;

					// This is needed, since otherwise the semantics would be that the if0Task task is being launched from within its own execution
					_task = nullptr;

					If0Task::executeNonInline(this, if0Task, cpu);
				} else {
					handleTask(cpu);
				}

				_task = nullptr;
			}
			CPUManager::checkIfMustReturnCPU(this);
		} else {
			// If no task is available, the CPUManager may want to idle this CPU
			CPUManager::executeCPUManagerPolicy(cpu, IDLE_CANDIDATE);
		}
		Instrument::workerThreadSpins();
	}

	// The thread should not have any task assigned at this point
	assert(_task == nullptr);

	// Runtime Tracking Point - The current thread is gonna shutdown
	TrackingPoints::threadWillShutdown();

	ThreadManager::addShutdownThread(this);
}

void WorkerThread::handleTask(CPU *cpu)
{
	assert(_task != nullptr);
	assert(cpu != nullptr);

	// Only for source taskfors
	if (_task->isTaskforSource()) {
		Taskfor *source = (Taskfor *) _task;

		// The scheduler set the chunk on the CPU preallocated taskfor
		if (cpu->getPreallocatedTaskfor()->getMyChunk() >= 0) {
			Taskfor *collaborator = LoopGenerator::createCollaborator(source, cpu);
			assert(collaborator->isRunnable());
			assert(collaborator->getMyChunk() >= 0);

			_task = collaborator;
		} else {
			// The taskfor source has no chunks available
			bool finished = source->notifyCollaboratorHasFinished();
			if (finished) {
				TaskFinalization::disposeTask(_task);
			}
			_task = nullptr;
		}
	}

	// Execute the task
	if (_task != nullptr) {
		executeTask(cpu);

		_task = nullptr;
	}
}

void WorkerThread::executeTask(CPU *cpu)
{
	assert(_task != nullptr);
	assert(cpu != nullptr);

	nanos6_address_translation_entry_t stackTranslationTable[SymbolTranslation::MAX_STACK_SYMBOLS];

	// For multi-implements cases
	uint8_t implementation = 0;
	for (implementation = 0; implementation < _task->getImplementationCount(); implementation++) {
		if (_task->getDeviceType(implementation) == nanos6_host_device) {
			break;
		}
	}
	assert(_task->getDeviceType(implementation) == nanos6_host_device);

	const size_t NUMAId = cpu->getNumaNodeId();
	MemoryPlace *memoryPlace = HardwareInfo::getMemoryPlace(nanos6_host_device, NUMAId);
	assert(memoryPlace != nullptr);

	_task->setThread(this);
	_task->setMemoryPlace(memoryPlace);

	Instrument::task_id_t taskId;
	if (_task->isTaskforCollaborator()) {
		taskId = _task->getParent()->getInstrumentationTaskId();
	} else {
		taskId = _task->getInstrumentationTaskId();
	}
	Instrument::ThreadInstrumentationContext instrumentationContext(
		taskId, cpu->getInstrumentationId(), _instrumentationId
	);

	if (_task->hasCode(implementation)) {
		size_t tableSize = 0;
		nanos6_address_translation_entry_t *translationTable =
			SymbolTranslation::generateTranslationTable(
				_task, cpu, stackTranslationTable, tableSize
			);

		// Runtime Tracking Point - A task starts its execution
		TrackingPoints::taskIsExecuting(_task);

		// Run the task
		std::atomic_thread_fence(std::memory_order_acquire);
		_task->body(translationTable, implementation);
		std::atomic_thread_fence(std::memory_order_release);

		// Update the CPU since the thread may have migrated
		cpu = getComputePlace();
		instrumentationContext.updateComputePlace(cpu->getInstrumentationId());

		// Runtime Tracking Point - A task has completed its execution (user code)
		TrackingPoints::taskCompletedUserCode(_task);

		// Free up all symbol translation
		if (tableSize > 0) {
			MemoryAllocator::free(translationTable, tableSize);
		}
	} else {
		// Runtime Tracking Point - A task has completed its execution (user code)
		TrackingPoints::taskCompletedUserCode(_task);
	}

	DataAccessRegistration::combineTaskReductions(_task, cpu);

	if (_task->markAsFinished(cpu)) {
		DataAccessRegistration::unregisterTaskDataAccesses(
			_task, cpu, cpu->getDependencyData()
		);

		TaskFinalization::taskFinished(_task, cpu);
		if (_task->markAsReleased()) {
			TaskFinalization::disposeTask(_task);
		}
	}
}
