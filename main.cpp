#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

//Custom Classes
#include "VulkanInstance.h"
#include "VulkanDevices.h"

class Application {
public: 
	void run() {
		//Initialize Vulkan
		initVulkan();
		mainLoop();
		cleanup();
	};

private: 
	//Handles instance creation and validation layers
	VulkanInstance instance; 
	//Handles physical, logical and graphicsQueue creation
	VulkanDevices devices;

	void initVulkan() {
		// Create VkInstance
		instance.createInstance();
		instance.setupDebugMessenger();

		//Get Vulkan Instance
		VkInstance vulkanInstance = instance.getInstance();

		// Pick physical device
		devices.pickPhysicalDevice(vulkanInstance);
		// Create logical device
		devices.createLogicalDevice(instance.validationLayers);
	};

	void mainLoop() {
		while (!glfwWindowShouldClose(instance.window)) {
			glfwPollEvents();
		}
	};

	void cleanup() {
		//Cleans up devices
		devices.~VulkanDevices();
		//Cleans up instance, debug messenger and GLFW window
		instance.~VulkanInstance();
	};
};

int main() {
	Application app; 

	try {
		app.run(); 
	} catch (const std::exception &e){
		return EXIT_FAILURE; 
	}

	return EXIT_FAILURE; 
};