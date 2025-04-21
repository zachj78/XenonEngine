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
	VkInstance instance = VK_NULL_HANDLE; // Vulkan Instance

	//Initialize GLFW window on instance construction
	VulkanInstance();
	void cleanup();

	//Main functions
	void createInstance();
	void setupDebugMessenger();

	//Getter functions

	//Structs
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

private: 
	VkDebugUtilsMessengerEXT debugMessenger; //Vulkan Debug Messenger
};

#endif