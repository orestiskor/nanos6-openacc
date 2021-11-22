/*
	This file is part of Nanos6 and is licensed under the terms contained in the COPYING file.

	Copyright (C) 2021 Barcelona Supercomputing Center (BSC)
*/

#include <config.h>

#include "EnergyManager.hpp"

#include "hardware/HardwareInfo.hpp"
#include "hwinfo/HostInfo.hpp"
#include "lowlevel/FatalErrorHandler.hpp"
#include "support/config/ConfigVariable.hpp"

#ifdef USE_CUDA
#include "hardware/device/cuda/CUDADeviceInfo.hpp"
#endif

#ifdef USE_OPENACC
#include "hardware/device/openacc/OpenAccDeviceInfo.hpp"
#endif

RWSpinLock EnergyManager::_lock;
std::map<std::string, std::vector<struct EnergyManager::ImplemEnergyData>> EnergyManager::_energyDirectory;
float EnergyManager::_availability[nanos6_device_type_num];
bool EnergyManager::_enabled;
ConfigVariable<std::string> EnergyManager::_energyFile("energy.filename");

void EnergyManager::parseXpu(JsonNode<> &node, struct EnergyManager::ImplemEnergyData *data)
{
	float sum = 0;
	if (data->device_type == nanos6_host_device) {
		node.traverseChildrenNodes(
			[&](const std::string &in_name, JsonNode<> &in_node) {
				JsonNode<> tpn;
				float tmp;
				if (in_node.childNodeExists("package")) {
					tpn = in_node.getChildNode("package");
					bool parsed = tpn.getData("total", tmp);
					sum += tmp;
				}
			}
		);
	}
	else {
		node.traverseChildrenNodes(
			[&](const std::string &in_name, JsonNode<> &in_node) {
				JsonNode<> tpn;
				float tmp;
				if (in_node.childNodeExists("board")) {
					tpn = in_node.getChildNode("board");
					bool parsed = tpn.getData("total", tmp);
					sum += tmp;
				}
			}
		);
	}
	data->energy = sum;
}

void EnergyManager::parseExecutions(JsonNode<> &node,
	std::vector<struct EnergyManager::ImplemEnergyData> &data,
	nanos6_device_t devType)
{
	node.traverseChildrenNodes(
		[&](const std::string &in_name, JsonNode<> &in_node) {
			float time;
			if (in_node.dataExists("time")) {
				struct ImplemEnergyData *n_data = new struct ImplemEnergyData;
				n_data->device_type = devType;
				bool parsed = in_node.getData("time", time);
				std::string jstype;

				switch ((int)devType) {
					case nanos6_cuda_device:
						jstype = "gpu";
						n_data->device_type = nanos6_cuda_device;
						break;
					case nanos6_openacc_device:
						jstype = "gpu";
						n_data->device_type = nanos6_openacc_device;
						break;
					default:
						jstype = "cpu";
						n_data->device_type = nanos6_host_device;
						break;
				}


				n_data->time = time;

				// Add here the parser of internal subtree per dev type
				JsonNode<> xpu = in_node.getChildNode(jstype);

				parseXpu(xpu, n_data);

				data.push_back(*n_data);
			}
		}
	);
}

void EnergyManager::parseSections(JsonNode<> &node,
	std::vector<struct EnergyManager::ImplemEnergyData> &data)
{
	node.traverseChildrenNodes(
		[&](const std::string &in_name, JsonNode<> &in_node) {
			std::string label;
			std::string devstr;
			nanos6_device_t devType;
			bool parsed = in_node.getData<std::string>("label", label);
			if (!parsed)
				FatalErrorHandler();

			parsed = in_node.getData<std::string>("extra", devstr);

			if (devstr == "cuda") {
				devType = nanos6_cuda_device;
			} else if (devstr == "openacc") {
				devType = nanos6_openacc_device;
			} else {
				devType = nanos6_host_device;
			}

			JsonNode<> executions = in_node.getChildNode("executions");
			parseExecutions(executions, data, devType);
		}
	);
}

bool EnergyManager::loadDataFile()
{
	if (!_energyFile.isPresent())
		return false;

	std::string file = _energyFile.getValue().c_str();
	JsonFile dataFile = JsonFile(file);
	if (dataFile.fileExists()) {
		dataFile.loadData();

		// Navigate through the file and extract energy profiling info
		dataFile.getRootNode()->traverseChildrenNodes(
			[&](const std::string &name, JsonNode<> &node) {
				if (name == "groups") {
					node.traverseChildrenNodes(
						[&](const std::string &in_name, JsonNode<> &in_node) {
							std ::string label;
							bool parsed = in_node.getData<std::string>("label", label);
							JsonNode<> sections = in_node.getChildNode("sections");
							std::vector<struct ImplemEnergyData> eData;
							parseSections(sections, eData);
							if (!eData.empty()) {
								_energyDirectory.insert(std::make_pair(label, eData));
							}
						}
					);
				}
			}
		);

	}

	else {
		FatalErrorHandler::fail("Energy profiling file not found"); // FIXME optional
		return false;
	}

	return (!_energyDirectory.empty());
}

nanos6_device_t EnergyManager::selectDevice(Task *task)
{
	assert(task != nullptr);
	if (!_enabled)
		return (nanos6_device_t)task->getDeviceType();

	uint8_t implementationCount = task->getImplementationCount();
	nanos6_device_t selectedDevice;
	// Device types of available implementations
	nanos6_device_t availableImplements[implementationCount];

	std::string label = task->getLabel();


	// populate available implementations | necessary? All optional ideas here
	for (uint8_t i = 0; i < implementationCount; i++) {
		assert(task->hasCode(i)); // probably redundant TODO: check and remove
		availableImplements[i] = (nanos6_device_t)task->getDeviceType(i);
		// Update the availability of a given device type?
	}

	// if (_energyDirectory.find(label) == _energyDirectory.end())
	selectedDevice = nanos6_host_device; // TODO Get the "best" score
	return selectedDevice;
}
