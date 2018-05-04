/*
	This file is part of Nanos6 and is licensed under the terms contained in the COPYING file.
	
	Copyright (C) 2015-2017 Barcelona Supercomputing Center (BSC)
*/

#ifndef INSTRUMENT_EXTRAE_TASK_EXECUTION_HPP
#define INSTRUMENT_EXTRAE_TASK_EXECUTION_HPP


#include "../api/InstrumentTaskExecution.hpp"
#include "../support/InstrumentThreadLocalDataSupport.hpp"
#include "InstrumentExtrae.hpp"

#include <cassert>


namespace Instrument {
	inline void startTask(
		task_id_t taskId,
		__attribute__((unused)) InstrumentationContext const &context
	) {
		extrae_combined_events_t ce;
		
		ce.HardwareCounters = 1;
		ce.Callers = 0;
		ce.UserFunction = EXTRAE_USER_FUNCTION_NONE;
		ce.nEvents = 5;
		ce.nCommunications = 0;
		
		if (!_sampleTaskCount) {
			ce.nEvents += 1;
		}
		
		if (_emitGraph) {
			taskId._taskInfo->_lock.lock();
			ce.nCommunications += taskId._taskInfo->_predecessors.size();
		}
		
		ce.Types  = (extrae_type_t *)  alloca (ce.nEvents * sizeof (extrae_type_t) );
		ce.Values = (extrae_value_t *) alloca (ce.nEvents * sizeof (extrae_value_t));
		
		if (ce.nCommunications > 0) {
			ce.Communications = (extrae_user_communication_t *) alloca(sizeof(extrae_user_communication_t) * ce.nCommunications);
		}
		
		ce.Types[0] = (extrae_type_t) EventType::RUNTIME_STATE;
		ce.Values[0] = (extrae_value_t) NANOS_RUNNING;
		
		ce.Types[1] = (extrae_type_t) EventType::RUNNING_CODE_LOCATION;
		ce.Values[1] = (extrae_value_t) taskId._taskInfo->_taskInfo->implementations[0].run;
		
		ce.Types[2] = (extrae_type_t) EventType::NESTING_LEVEL;
		ce.Values[2] = (extrae_value_t) taskId._taskInfo->_nestingLevel;
		
		ce.Types[3] = (extrae_type_t) EventType::TASK_INSTANCE_ID;
		ce.Values[3] = (extrae_value_t) taskId._taskInfo->_taskId;
		
		ce.Types[4] = (extrae_type_t) EventType::PRIORITY;
		ce.Values[4] = (extrae_value_t) taskId._taskInfo->_priority;
		
		if (_emitGraph) {
			int index = 0;
			for (auto const &taskAndTag : taskId._taskInfo->_predecessors) {
				ce.Communications[index].type = EXTRAE_USER_RECV;
				ce.Communications[index].tag = taskAndTag.second;
				ce.Communications[index].size = (taskAndTag.first << 32) + taskId._taskInfo->_taskId;
				ce.Communications[index].partner = EXTRAE_COMM_PARTNER_MYSELF;
				ce.Communications[index].id = (taskAndTag.first << 32) + taskId._taskInfo->_taskId;
				index++;
			}
			taskId._taskInfo->_predecessors.clear();
			taskId._taskInfo->_lock.unlock();
		}
		
		size_t readyTasks = --_readyTasks;
		if (!_sampleTaskCount) {
			ce.Types[5] = (extrae_type_t) EventType::READY_TASKS;
			ce.Values[5] = (extrae_value_t) readyTasks;
			
			// This counter is not so reliable, so try to skip underflows
			if (((signed long long) ce.Values[5]) < 0) {
				ce.Values[5] = 0;
			}
		}
		
		ThreadLocalData &threadLocal = getThreadLocalData();
		threadLocal._nestingLevels.push_back(taskId._taskInfo->_nestingLevel);
		
		if (_traceAsThreads) {
			_extraeThreadCountLock.readLock();
		}
		Extrae_emit_CombinedEvents ( &ce );
		if (_traceAsThreads) {
			_extraeThreadCountLock.readUnlock();
		}
	}
	
	
	inline void returnToTask(
		__attribute__((unused)) task_id_t taskId,
		__attribute__((unused)) InstrumentationContext const &context
	) {
		extrae_combined_events_t ce;
		
		ce.HardwareCounters = 1;
		ce.Callers = 0;
		ce.UserFunction = EXTRAE_USER_FUNCTION_NONE;
		ce.nEvents = 5;
		ce.nCommunications = 0;
		
		if (!_sampleTaskCount) {
			ce.nEvents += 1;
		}
		
		if (_emitGraph) {
			taskId._taskInfo->_lock.lock();
			ce.nCommunications += taskId._taskInfo->_predecessors.size();
		}
		
		ce.Types  = (extrae_type_t *)  alloca (ce.nEvents * sizeof (extrae_type_t) );
		ce.Values = (extrae_value_t *) alloca (ce.nEvents * sizeof (extrae_value_t));
		
		if (ce.nCommunications > 0) {
			ce.Communications = (extrae_user_communication_t *) alloca(sizeof(extrae_user_communication_t) * ce.nCommunications);
		}
		
		ce.Types[0] = (extrae_type_t) EventType::RUNTIME_STATE;
		ce.Values[0] = (extrae_value_t) NANOS_RUNNING;
		
		ce.Types[1] = (extrae_type_t) EventType::RUNNING_CODE_LOCATION;
		ce.Values[1] = (extrae_value_t) taskId._taskInfo->_taskInfo->implementations[0].run;
		
		ce.Types[2] = (extrae_type_t) EventType::NESTING_LEVEL;
		ce.Values[2] = (extrae_value_t) taskId._taskInfo->_nestingLevel;
		
		ce.Types[3] = (extrae_type_t) EventType::TASK_INSTANCE_ID;
		ce.Values[3] = (extrae_value_t) taskId._taskInfo->_taskId;
		
		ce.Types[4] = (extrae_type_t) EventType::PRIORITY;
		ce.Values[4] = (extrae_value_t) taskId._taskInfo->_priority;
		
		size_t readyTasks = --_readyTasks;
		if (!_sampleTaskCount) {
			ce.Types[5] = (extrae_type_t) EventType::READY_TASKS;
			ce.Values[5] = (extrae_value_t) readyTasks;
			
			// This counter is not so reliable, so try to skip underflows
			if (((signed long long) ce.Values[5]) < 0) {
				ce.Values[5] = 0;
			}
		}
		
		if (_emitGraph) {
			int index = 0;
			for (auto const &taskAndTag : taskId._taskInfo->_predecessors) {
				ce.Communications[index].type = EXTRAE_USER_RECV;
				ce.Communications[index].tag = taskAndTag.second;
				ce.Communications[index].size = (taskAndTag.first << 32) + taskId._taskInfo->_taskId;
				ce.Communications[index].partner = EXTRAE_COMM_PARTNER_MYSELF;
				ce.Communications[index].id = (taskAndTag.first << 32) + taskId._taskInfo->_taskId;
				index++;
			}
			taskId._taskInfo->_predecessors.clear();
			taskId._taskInfo->_lock.unlock();
		}
		
		if (_traceAsThreads) {
			_extraeThreadCountLock.readLock();
		}
		Extrae_emit_CombinedEvents ( &ce );
		if (_traceAsThreads) {
			_extraeThreadCountLock.readUnlock();
		}
	}
	
	
	inline void endTask(
		task_id_t taskId,
		__attribute__((unused)) InstrumentationContext const &context
	) {
		extrae_combined_events_t ce;
		
		ce.HardwareCounters = 1;
		ce.Callers = 0;
		ce.UserFunction = EXTRAE_USER_FUNCTION_NONE;
		ce.nEvents = 5;
		ce.nCommunications = 0;
		
		if (!_sampleTaskCount) {
			ce.nEvents += 1;
		}
		
		size_t parentInTaskwait = 0;
		if (_emitGraph) {
			if ((taskId._taskInfo->_parent != nullptr) && taskId._taskInfo->_parent->_inTaskwait) {
				taskId._taskInfo->_parent->_lock.lock();
				if (taskId._taskInfo->_parent->_inTaskwait) {
					parentInTaskwait = taskId._taskInfo->_parent->_taskId;
					ce.nCommunications++;
					
					taskId._taskInfo->_parent->_predecessors.emplace(taskId._taskInfo->_taskId, control_dependency_tag);
				}
				taskId._taskInfo->_parent->_lock.unlock();
			}
		}
		
		ce.Types  = (extrae_type_t *)  alloca (ce.nEvents * sizeof (extrae_type_t) );
		ce.Values = (extrae_value_t *) alloca (ce.nEvents * sizeof (extrae_value_t));
		
		if (ce.nCommunications > 0) {
			ce.Communications = (extrae_user_communication_t *) alloca(sizeof(extrae_user_communication_t) * ce.nCommunications);
		}
		
		ce.Types[0] = (extrae_type_t) EventType::RUNTIME_STATE;
		ce.Values[0] = (extrae_value_t) NANOS_IDLE;
		
		ce.Types[1] = (extrae_type_t) EventType::RUNNING_CODE_LOCATION;
		ce.Values[1] = (extrae_value_t) nullptr;
		
		ce.Types[2] = (extrae_type_t) EventType::NESTING_LEVEL;
		ce.Values[2] = (extrae_value_t) nullptr;
		
		ce.Types[3] = (extrae_type_t) EventType::TASK_INSTANCE_ID;
		ce.Values[3] = (extrae_value_t) nullptr;
		
		ce.Types[4] = (extrae_type_t) EventType::PRIORITY;
		ce.Values[4] = (extrae_value_t) nullptr;
		
		if (parentInTaskwait != 0) {
			ce.Communications[0].type = EXTRAE_USER_SEND;
			ce.Communications[0].tag = control_dependency_tag;
			ce.Communications[0].size = (taskId._taskInfo->_taskId << 32) + parentInTaskwait;
			ce.Communications[0].partner = EXTRAE_COMM_PARTNER_MYSELF;
			ce.Communications[0].id = (taskId._taskInfo->_taskId << 32) + parentInTaskwait;
		}
		
		size_t liveTasks = --_liveTasks;
		if (!_sampleTaskCount) {
			ce.Types[5] = (extrae_type_t) EventType::LIVE_TASKS;
			ce.Values[5] = (extrae_value_t) liveTasks;
			
			// This counter is not so reliable, so try to skip underflows
			if (((signed long long) ce.Values[5]) < 0) {
				ce.Values[5] = 0;
			}
		}
		
		if (_traceAsThreads) {
			_extraeThreadCountLock.readLock();
		}
		Extrae_emit_CombinedEvents ( &ce );
		if (_traceAsThreads) {
			_extraeThreadCountLock.readUnlock();
		}
		
		ThreadLocalData &threadLocal = getThreadLocalData();
		assert(!threadLocal._nestingLevels.empty());
		threadLocal._nestingLevels.pop_back();
	}
	
	
	inline void destroyTask(
		__attribute__((unused)) task_id_t taskId,
		__attribute__((unused)) InstrumentationContext const &context
	) {
	}
}


#endif // INSTRUMENT_EXTRAE_TASK_EXECUTION_HPP
