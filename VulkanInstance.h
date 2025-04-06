#ifndef VULKAN_INSTANCE_H
#define VULKAN_INSTANCE_H

#include "config.h"
#include "helperFuncs.h"

#include <vector>
#include <optional>
#include <set>

class VulkanInstance {
public: 
	//Variables
	GLFWwindow* window = VK_NULL_HANDLE;

	//Initialize GLFW window on instance construction
	VulkanInstance();
	~VulkanInstance();

	//Main functions
	void createInstance();
	void setupDebugMessenger();

	//Getter functions
	const VkInstance getInstance() { return instance; };

	//Structs
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

private: 
	VkInstance instance = VK_NULL_HANDLE; // Vulkan Instance
	VkDebugUtilsMessengerEXT debugMessenger; //Vulkan Debug Messenger
};

#endif