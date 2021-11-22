/*
	This file is part of Nanos6 and is licensed under the terms contained in the COPYING file.

	Copyright (C) 2021 Barcelona Supercomputing Center (BSC)
*/

#ifndef ENERGY_MANAGER_HPP
#define ENERGY_MEM_MANAGER_HPP

#include "lowlevel/RWSpinLock.hpp"
#include "support/JsonFile.hpp"
#include "tasks/Task.hpp"

class EnergyManager {

	struct ImplemEnergyData
	{
		nanos6_device_t device_type;
		float energy; // in Joules, total
		float time;
		std::string variation; //optional
	};

private:

	// Associate the task label with a set of implementations and their energy data
	static std::map<std::string, std::vector<struct ImplemEnergyData>> _energyDirectory;

	static ConfigVariable<std::string> _energyFile;

	// This is PoC ONLY, a first very naive and irrelevant implementation!
	static float _availability[nanos6_device_type_num];

	static RWSpinLock _lock;

	static bool _enabled;

	// Update the "availability" for a given device type
	static void updateAvailability(nanos6_device_t devType)
	{
		if (_availability[devType] > 0.f) {
			// update


		}
		else {
			// set deviceavail to -1.0, but this is stupid, we may have an init step and populate devices
			// then decide if it's worth updating in the selectDevice(), so we don't enter this case in stupid occasions
		}

	}
 // TODO Move to cpp file


	// Parsing functions, called by loadDataFile(), following the structure of the INESC Energy-Profiler files

	// Parse CPU or GPU JSON nodes; For CPUs read the "package" information, for GPUs read "board"
	static void parseXpu(JsonNode<> &node, struct ImplemEnergyData *data);

	static void parseExecutions(JsonNode<> &node, std::vector<struct ImplemEnergyData> &data, nanos6_device_t devType);

	static void parseSections(JsonNode<> &node, std::vector<struct ImplemEnergyData> &data);

	// Reads and parses the Energy data, provided by the Energy-Profiler tool
	// Returns:
	//    true: If a file is present and parsed correctly. The Manager is enabled and will choose task implementations
	// 	  false: The Manager will not be enabled due to absence of data. Always pick the first implementation of a task
	static bool loadDataFile();

public:

	static void initialize()
	{
		_enabled = loadDataFile();
		if (_enabled) {

			for (int i = 0; i < nanos6_device_type_num; i++) {
				if (HardwareInfo::canDeviceRunTasks((nanos6_device_t)i)) {
					_availability[i] = 1.f;
				}
				else {
					// set negative and never change; ignore provided implementations
					_availability[i] = -1.f;
				}
			}
		}
	}

	static void shutdown()
	{
		for (auto entry : _energyDirectory) {
			for (auto it : entry.second) {
				delete &it;
			}
			delete &entry.second; //??
		}
	}

	static nanos6_device_t selectDevice(Task *task);

};

#endif // ENERGY_MANAGER_HPP
