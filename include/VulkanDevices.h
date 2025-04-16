#ifndef VULKAN_DEVICES_H
#define VULKAN_DEVICES_H

#include "config.h"
#include "cstm_types.h"

class VulkanDevices {
public: 
	//Destructor
	~VulkanDevices();

	//Main functions
	void pickPhysicalDevice(VkInstance &instance, VkSurfaceKHR &surface);
	void createLogicalDevice(const std::vector<const char*> validationLayers, VkSurfaceKHR &surface);

	//Getter functions
	VkPhysicalDevice getPhysicalDevice() { return physicalDevice; };
	VkDevice getLogicalDevice() { return device; };

	//Logger functions

	//Variables
	//Device extensions array - specifies extensions our physical device needs
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

private: 
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkQueue presentQueue = VK_NULL_HANDLE;
};

#endif