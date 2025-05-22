#ifndef VULKAN_INSTANCE_H
#define VULKAN_INSTANCE_H

#include "config.h"
#include "helperFuncs.h"
#include "DebugManager.h"

#include <vector>
#include <optional>
#include <set>

class VulkanInstance {
public: 
	bool framebufferResized = false;

	//Initialize GLFW window on instance construction
	VulkanInstance(std::shared_ptr<DebugManager> debugManager);
	void cleanup();

	//Main functions
	void createInstance();
	void createSurface();

	//Getter functions
	GLFWwindow* getWindowPtr() const { return window; };
	VkInstance getInstance() const { return instance; };
	VkSurfaceKHR getSurface() const { return surface; };



private: 
	std::shared_ptr<DebugManager> instance_debugManager;

	GLFWwindow* window = VK_NULL_HANDLE;
	VkInstance instance = VK_NULL_HANDLE; // Vulkan Instance
	VkSurfaceKHR surface;
};

#endif