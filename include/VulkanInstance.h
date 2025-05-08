#ifndef VULKAN_INSTANCE_H
#define VULKAN_INSTANCE_H

#include "config.h"
#include "helperFuncs.h"

#include <vector>
#include <optional>
#include <set>

class VulkanInstance {
public: 
	bool framebufferResized = false;

	//Initialize GLFW window on instance construction
	VulkanInstance();
	void cleanup();

	//Main functions
	void createInstance();
	void setupDebugMessenger();
	void createSurface();

	//Getter functions
	GLFWwindow* getWindowPtr() const { return window; };
	VkInstance getInstance() const { return instance; };
	VkSurfaceKHR getSurface() const { return surface; };

	//Structs
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

private: 
	VkDebugUtilsMessengerEXT debugMessenger; //Vulkan Debug Messenger

	GLFWwindow* window = VK_NULL_HANDLE;
	VkInstance instance = VK_NULL_HANDLE; // Vulkan Instance
	VkSurfaceKHR surface;
};

#endif