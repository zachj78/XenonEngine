#ifndef VULKAN_DEVICES_H
#define VULKAN_DEVICES_H

#include "config.h"

class VulkanDevices {
public: 
	//Destructor
	~VulkanDevices();

	//Main functions
	void pickPhysicalDevice(VkInstance instance);
	void createLogicalDevice(const std::vector<const char*> validationLayers);

	//Getter functions
	VkPhysicalDevice getPhysicalDevice() { return physicalDevice; };
	VkDevice getLogicalDevice() { return device; };

private: 
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkQueue graphicsQueue = VK_NULL_HANDLE;
};

#endif