/*
	This file is part of Nanos6 and is licensed under the terms contained in the COPYING file.

	Copyright (C) 2020 Barcelona Supercomputing Center (BSC)
*/

#include <algorithm>

#include "DeviceMemManager.hpp"

#include "hardware/HardwareInfo.hpp"

#include <DataAccessRegistration.hpp>
#include <DataAccessRegistrationImplementation.hpp>

const size_t DeviceMemManager::totalDevices = CUDAFunctions::getDeviceCount();
RWSpinLock DeviceMemManager::_lock;
std::map<void *, DeviceMemManager::DeviceMemEntry> DeviceMemManager::_directory;
bool DeviceMemManager::_regions_deps;

void DeviceMemManager::initialize(void)
{
	static ConfigVariable<std::string> dependencies("version.dependencies");

	if (dependencies.getValue().compare("regions") == 0)
		_regions_deps = true;
	else
		_regions_deps = false;
}

void *DeviceMemManager::alloc(size_t size, int deviceNum)
{
	assert((size > 0) && (deviceNum >= 0) && (totalDevices > 0));

	// deviceNum temporarily will be just (deviceNum % totalDevices)

	// Our very sophisticated hash...
	deviceNum = deviceNum % (int)totalDevices;

	// For starters, only play with CUDAAttached mode; Global is automatic in simple malloc
	// substitution by PGI + UnifiedMemory flag.
	void *ret = CUDAFunctions::mallocManaged(size, deviceNum, CUDAFunctions::CUDAAttached);
	// CUDAErrorHandler will abort in case of failure; assume correct result

	DeviceMemEntry entry = {size, deviceNum};
	_lock.writeLock();
	_directory.insert(std::make_pair(ret, entry));
	_lock.writeUnlock();

	return ret;
}

void DeviceMemManager::free(void *ptr)
{
	assert(_directory.find(ptr) != _directory.end());

	_lock.writeLock();
	_directory.erase(ptr);
	_lock.writeUnlock();
	CUDAFunctions::free(ptr);
}

bool DeviceMemManager::getEntry(void *ptr, DeviceMemManager::DeviceMemEntry *res)
{
	assert(ptr != nullptr && res != nullptr);
	*res = {0, 0};
	bool ret = false;
	_lock.readLock();

	// If we have regions dependencies, it can be the case that we are looking for an access
	// that is not the first element/pointer of the allocated memory, e.g. our dependency is
	// A[42:256] instead of A (like in discrete mode)
	//
	// For this, we have to look-up if the given pointer falls anywhere within one of the allocated
	// regions of the directory.
	if (_regions_deps) {
		bool stop = false;
		auto it  = _directory.begin();
		while (it != _directory.end() && !stop) {
			if ((size_t)ptr >= (size_t)it->first &&
					(size_t)ptr <= (it->second.size + (size_t)it->first) ) {
				res->size = it->second.size - ((size_t)ptr - (size_t)(it->first)); // probably not needed to calculate this...
				res->deviceNum = it->second.deviceNum;
				stop = true;
				ret = true;
			}
			++it;
			// We hope the map is sorted and the heap grows upwards :|
			// If the pointer is not found to be contained AND the next entry is higher,
			// it probably means we can abandon the search here
			if ((size_t)it->first > (size_t)ptr)
				stop = true;
		}
	}
	else {
		auto it = _directory.find(ptr);

		if (it != _directory.end()) {
			*res = {it->second.size, it->second.deviceNum};
			ret = true;
		}
	}
	_lock.readUnlock();

	return ret;
}

size_t DeviceMemManager::computeDeviceAffinity(Task *task)
{
	size_t deviceScore[totalDevices] = { };

	DataAccessRegistration::processAllDataAccesses(task,
		[&](const DataAccess *access) -> bool {
			if (access->getType() != REDUCTION_ACCESS_TYPE && !access->isWeak()) {
				void *address = access->getAccessRegion().getStartAddress();
				DeviceMemEntry dirEntry = {0, 0};
				if (getEntry(address, &dirEntry)) {
					int device = dirEntry.deviceNum;
					size_t accessSize = access->getAccessRegion().getSize();
					deviceScore[device] += accessSize;
				}
			}
			return true;
		}
	);

	size_t ret = std::distance(deviceScore, std::max_element(deviceScore, deviceScore + totalDevices));
	return ret;

}

