#pragma once
#ifndef DEVICES_H
#define DEVICES_H

#include "config.h"
#include "cstm_types.h"
#include "VulkanInstance.h"

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

	//cleanup function
	void cleanup();

	//Logger functions

	//Variables
	//Device extensions array - specifies extensions our physical device needs
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

private:
	//Injected Vulkan Core Components
	std::shared_ptr<VulkanInstance> dev_instance;
	std::shared_ptr<DebugManager> dev_debugManager;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkQueue presentQueue = VK_NULL_HANDLE;

	//Helper functions
	const bool checkDeviceExtensionSupport(VkPhysicalDevice potentialDevice);
	const bool isDeviceSuitable(VkPhysicalDevice potentialDevice);
	// this last logger function should be moved to a logger utility class
	void logPhysicalDevice() const;
};

#endif