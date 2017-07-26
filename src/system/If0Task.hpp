/*
	This file is part of Nanos6 and is licensed under the terms contained in the COPYING file.
	
	Copyright (C) 2015-2017 Barcelona Supercomputing Center (BSC)
*/

#ifndef IF0_TASK_HPP
#define IF0_TASK_HPP

#include <cassert>

#include <InstrumentTaskWait.hpp>
#include <InstrumentTaskStatus.hpp>

#include "executors/threads/CPU.hpp"
#include "executors/threads/WorkerThread.hpp"
#include "executors/threads/ThreadManager.hpp"
#include "scheduling/Scheduler.hpp"
#include "tasks/Task.hpp"


class ComputePlace;


namespace If0Task {
	inline void waitForIf0Task(WorkerThread *currentThread, Task *currentTask, Task *if0Task, ComputePlace *computePlace)
	{
		assert(currentThread != nullptr);
		assert(currentTask != nullptr);
		assert(if0Task != nullptr);
		assert(computePlace != nullptr);
		
		CPU *cpu = static_cast<CPU *>(computePlace);
		
		Instrument::enterTaskWait(currentTask->getInstrumentationTaskId(), if0Task->getTaskInvokationInfo()->invocation_source, if0Task->getInstrumentationTaskId());
		
		WorkerThread *replacementThread = ThreadManager::getIdleThread(cpu);
		
		Instrument::taskIsBlocked(currentTask->getInstrumentationTaskId(), Instrument::in_taskwait_blocking_reason);
		currentThread->switchTo(replacementThread);
		
		Instrument::exitTaskWait(currentTask->getInstrumentationTaskId());
		Instrument::taskIsExecuting(currentTask->getInstrumentationTaskId());
	}
	
	
	inline void executeInline(
		WorkerThread *currentThread, Task *currentTask, Task *if0Task,
		ComputePlace *computePlace
	) {
		assert(currentThread != nullptr);
		assert(currentTask != nullptr);
		assert(if0Task != nullptr);
		assert(if0Task->getParent() == currentTask);
		assert(computePlace != nullptr);
		
		bool hasCode = if0Task->hasCode();
		
		Instrument::enterTaskWait(currentTask->getInstrumentationTaskId(), if0Task->getTaskInvokationInfo()->invocation_source, if0Task->getInstrumentationTaskId());
		if (hasCode) {
			Instrument::taskIsBlocked(currentTask->getInstrumentationTaskId(), Instrument::in_taskwait_blocking_reason);
		}
		
		currentThread->handleTask((CPU *) computePlace, if0Task);
		
		Instrument::exitTaskWait(currentTask->getInstrumentationTaskId());
		
		if (hasCode) {
			Instrument::taskIsExecuting(currentTask->getInstrumentationTaskId());
		}
	}
	
	
	inline void executeNonInline(
		WorkerThread *currentThread, Task *if0Task,
		ComputePlace *computePlace
	) {
		assert(currentThread != nullptr);
		assert(if0Task != nullptr);
		assert(computePlace != nullptr);
		
		assert(if0Task->isIf0());
		
		Task *parent = if0Task->getParent();
		assert(parent != nullptr);
		
		currentThread->handleTask((CPU *) computePlace, if0Task);
		
		// The thread can migrate during the execution of the task
		computePlace = currentThread->getComputePlace();
		
		Scheduler::taskGetsUnblocked(parent, computePlace);
	}
	
}


#endif // IF0_TASK_HPP
