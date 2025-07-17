#pragma once
#ifndef DEVICES_H
#define DEVICES_H

#include "Utils/config.h"
#include "Utils/cstm_types.h"
#include "Core/VulkanInstance.h"

struct Capabilities {
	bool supportsDescriptorIndexing = false; 
	bool supportsBindless = false; 
	bool supportsAnisotrophy = false;

	bool runtimeDescriptorArray = false;
	bool shaderSampledImageArrayNonUniformIndexing = false;
	bool descriptorBindingPartiallyBound = false;
	bool descriptorBindingVariableDescriptorCount = false;

	uint32_t maxUpdateAfterBindDescriptorsInAllPools = 0;

	void query(VkPhysicalDevice potentialDevice);
};

class Devices {
public: 
	Devices(std::shared_ptr<VulkanInstance> instance, std::shared_ptr<DebugManager> debugManager)
		: dev_instance(instance), dev_debugManager(debugManager) {
		std::cout << "Constructed `Devices`" << std::endl;
	};

	//Main functions
	void pickPhysicalDevice();
	void createLogicalDevice();

	//Getter functions
	VkPhysicalDevice getPhysicalDevice() { return physicalDevice; };
	VkDevice getLogicalDevice() { return device; };
	VkQueue getGraphicsQueue() { return graphicsQueue; };
	VkQueue getPresentQueue() { return presentQueue; };
	Capabilities getDeviceCaps() { return deviceCaps; };

	QueueFamilyIndices getQueueFamilies() const { return queueFamilies; };

	//cleanup function
	void cleanup();

	//Logger functions
	std::vector<const char*> deviceExtensions = {};


private:
	//Injected Vulkan Core Components
	std::shared_ptr<VulkanInstance> dev_instance;
	std::shared_ptr<DebugManager> dev_debugManager;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkQueue presentQueue = VK_NULL_HANDLE;

	QueueFamilyIndices queueFamilies;

	//Capabilities
	Capabilities deviceCaps;

	//Helper functions
	const bool checkDeviceExtensionSupport(VkPhysicalDevice potentialDevice);
	const bool rateDeviceSuitability(VkPhysicalDevice potentialDevice, int currentGreatestDeviceScore);
	// this last logger function should be moved to a logger utility class
	void logPhysicalDevice() const;
};

#endif