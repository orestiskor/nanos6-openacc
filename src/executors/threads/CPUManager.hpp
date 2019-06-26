/*
	This file is part of Nanos6 and is licensed under the terms contained in the COPYING file.
	
	Copyright (C) 2015-2017 Barcelona Supercomputing Center (BSC)
*/

#ifndef CPU_MANAGER_HPP
#define CPU_MANAGER_HPP


#include <mutex>
#include <atomic>

#include <boost/dynamic_bitset.hpp>

#include "hardware/places/ComputePlace.hpp"
#include "lowlevel/SpinLock.hpp"
#include "lowlevel/FatalErrorHandler.hpp"

#include "CPU.hpp"


class CPUManager {
private:
	//! \brief Available CPUs indexed by virtual CPU identifier
	static std::vector<CPU *> _cpus;
	
	//! \brief indicates if the thread manager has finished initializing the CPUs
	static std::atomic<bool> _finishedCPUInitialization;
	
	//! \brief threads blocked due to idleness
	static boost::dynamic_bitset<> _idleCPUs;
	
	//! \brief NUMA node CPU mask
	static std::vector<boost::dynamic_bitset<>> _NUMANodeMask;
	
	//! \brief Map from system to virtual CPU id
	static std::vector<size_t> _systemToVirtualCPUId;
	
	static SpinLock _idleCPUsLock;
	
	static void reportInformation(size_t numSystemCPUs, size_t numNUMANodes);
	
public:
	static void preinitialize();
	
	static void initialize();
	
	//! \brief get the CPU object assigned to a given numerical system CPU identifier
	static inline CPU *getCPU(size_t systemCPUId);
	
	//! \brief get the maximum number of CPUs that will be used
	static inline long getTotalCPUs();
	
	//! \brief check if initialization has finished
	static inline bool hasFinishedInitialization();
	
	//! \brief get a reference to the list of CPUs
	static inline std::vector<CPU *> const &getCPUListReference();

	//! \brief mark a CPU as idle
	static inline void cpuBecomesIdle(CPU *cpu);

	//! \brief get an idle CPU
	static inline CPU *getIdleCPU();
	
	//! \brief get all idle CPUs
	static inline void getIdleCPUs(std::vector<CPU *> &idleCPUs);
	
	//! \brief get an idle CPU from a specific NUMA node
	static inline CPU *getIdleNUMANodeCPU(size_t NUMANodeId);
	
	//! \brief mark a CPU as not being idle (if possible)
	static inline bool unidleCPU(CPU *cpu);
};


inline CPU *CPUManager::getCPU(size_t systemCPUId)
{
	// _cpus is sorted by virtual ID
	assert(_systemToVirtualCPUId.size() > systemCPUId);
	size_t virtualCPUId = _systemToVirtualCPUId[systemCPUId];
	
	return _cpus[virtualCPUId];
}

inline long CPUManager::getTotalCPUs()
{
	return _cpus.size();
}

inline bool CPUManager::hasFinishedInitialization()
{
	return _finishedCPUInitialization;
}


inline std::vector<CPU *> const &CPUManager::getCPUListReference()
{
	return _cpus;
}


inline void CPUManager::cpuBecomesIdle(CPU *cpu)
{
	std::lock_guard<SpinLock> guard(_idleCPUsLock);
	_idleCPUs[cpu->getIndex()] = true;
}


inline CPU *CPUManager::getIdleCPU()
{
	std::lock_guard<SpinLock> guard(_idleCPUsLock);
	boost::dynamic_bitset<>::size_type idleCPU = _idleCPUs.find_first();
	if (idleCPU != boost::dynamic_bitset<>::npos) {
		_idleCPUs[idleCPU] = false;
		return _cpus[idleCPU];
	} else {
		return nullptr;
	}
}

inline void CPUManager::getIdleCPUs(std::vector<CPU *> &idleCPUs)
{
	assert(idleCPUs.empty());
	
	std::lock_guard<SpinLock> guard(_idleCPUsLock);
	boost::dynamic_bitset<>::size_type idleCPU = _idleCPUs.find_first();
	while (idleCPU != boost::dynamic_bitset<>::npos) {
		_idleCPUs[idleCPU] = false;
		idleCPUs.push_back(_cpus[idleCPU]);
		idleCPU = _idleCPUs.find_next(idleCPU);
	}
}

inline CPU *CPUManager::getIdleNUMANodeCPU(size_t NUMANodeId)
{
	std::lock_guard<SpinLock> guard(_idleCPUsLock);
	boost::dynamic_bitset<> tmpIdleCPUs = _idleCPUs & _NUMANodeMask[NUMANodeId];
	boost::dynamic_bitset<>::size_type idleCPU = tmpIdleCPUs.find_first();
	if (idleCPU != boost::dynamic_bitset<>::npos) {
		_idleCPUs[idleCPU] = false;
		return _cpus[idleCPU];
	} else {
		return nullptr;
	}
}


inline bool CPUManager::unidleCPU(CPU *cpu)
{
	assert(cpu != nullptr);
	
	std::lock_guard<SpinLock> guard(_idleCPUsLock);
	if (_idleCPUs[cpu->getIndex()]) {
		_idleCPUs[cpu->getIndex()] = false;
		return true;
	} else {
		return false;
	}
}

#endif // CPU_MANAGER_HPP
