/*
	This file is part of Nanos6 and is licensed under the terms contained in the COPYING file.

	Copyright (C) 2020 Barcelona Supercomputing Center (BSC)
*/

#ifndef DEVICE_MEM_MANAGER_HPP
#define DEVICE_MEM_MANAGER_HPP

#include <map>

#include "hardware/device/cuda/CUDAFunctions.hpp"
#include "lowlevel/RWSpinLock.hpp"
#include "tasks/Task.hpp"

class DeviceMemManager {

	struct DeviceMemEntry {
		size_t size;
		int deviceNum;
	};

private:
	static std::map<void *, DeviceMemEntry> _directory;

	static RWSpinLock _lock;

	static const size_t totalDevices;

	static bool _regions_deps; // true for regions, false for discrete

	// Get _directory entry data of a given pointer-symbol, if it exists in the directory
	// returns: bool, with 'contains' semantics; the idea is to be used both as 'contains' and entry getter.
	// param res: the result Entry. It makes sense to check the res entry if the return value is true
	// 			(will be a {0,0} entry otherwise)
	static bool getEntry(void *ptr, DeviceMemEntry *res);

public:
	static void initialize();

	static void shutdown()
	{
	}

	// Allocate Unified Memory (host+device at once) and add an entry in _directory
	static void *alloc(size_t size, int deviceNum);

	// Free symbol from Unified Memory and delete its entry from _directory
	static void free(void *ptr);

	// Given a -device- task, compute the device-affinity (device_num to execute on) based on dependencies
	static size_t computeDeviceAffinity(Task *task);

};

#endif // DEVICE_MEM_MANAGER_HPP

