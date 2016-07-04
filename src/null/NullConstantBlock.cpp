// This is for posix_memalign
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

#include "api/nanos6_rt_interface.h"
#include "lowlevel/FatalErrorHandler.hpp"

#include <cassert>
#include <cstdlib>
#include <iostream>


#define DATA_ALIGNMENT_SIZE sizeof(void *)
#define TASK_ALIGNMENT 128


static bool _inFinal = false;


class NullTask {
public:
	void *_argsBlock;
	nanos_task_info *_taskInfo;
	size_t _flags;
	
	NullTask(
		void *argsBlock,
		nanos_task_info *taskInfo,
		size_t flags
	)
		: _argsBlock(argsBlock),
		_taskInfo(taskInfo),
		_flags(flags)
	{
	}
};


void nanos_create_task(
	nanos_task_info *taskInfo,
	__attribute__((unused)) nanos_task_invocation_info *taskInvocationInfo,
	size_t args_block_size,
	void **args_block_pointer,
	void **task_pointer,
	size_t flags
) {
	static char __attribute__((aligned(TASK_ALIGNMENT))) theMem[16384];
	
	// Alignment fixup
	size_t missalignment = args_block_size & (DATA_ALIGNMENT_SIZE - 1);
	size_t correction = (DATA_ALIGNMENT_SIZE - missalignment) & (DATA_ALIGNMENT_SIZE - 1);
	args_block_size += correction;
	
	// Allocation and layout
	*args_block_pointer = (void *) theMem;
	
	// Operate directly over references to the user side variables
	void *&args_block = *args_block_pointer;
	void *&task = *task_pointer;
	
	task = (char *)args_block + args_block_size;
	
	// Construct the Task object
	new (task) NullTask(args_block, taskInfo, flags);
}


void nanos_submit_task(void *taskHandle)
{
	NullTask *task = (NullTask *) taskHandle;
	assert(task != nullptr);
	
	bool wasInFinal = _inFinal;
	assert(task->_taskInfo != nullptr);
	if ((task->_flags & nanos_final_task)) {
		_inFinal = true;
		task->_taskInfo->run_final(task->_argsBlock);
	} else {
		_inFinal = false;
		task->_taskInfo->run(task->_argsBlock);
	}
	_inFinal = wasInFinal;
	
	task->~NullTask();
}


signed int nanos_in_final(void)
{
	return _inFinal;
}


