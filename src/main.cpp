#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

//Custom Classes
#include "../include/VulkanInstance.h"
#include "../include/VulkanDevices.h"
#include "../include/GraphicsPipeline.h"


class Application {
public:
	void run() {
		//Initialize Vulkan
		initVulkan();
		mainLoop();
		cleanup();
	};

private:
	// -- Initialize pointers to custom classes -- 
	//Handles instance creation and validation layers
	std::unique_ptr<VulkanInstance> instance;
	//Handles physical, logical and graphicsQueue creation
	std::unique_ptr<VulkanDevices> devices;
	//Handles VkSurface and VkSwapchainKHR
	std::unique_ptr<GraphicsPipeline> graphicsPipeline;

	void initVulkan() {
		// Create VkInstance
		instance = std::make_unique<VulkanInstance>();
		instance->createInstance();
		instance->setupDebugMessenger();

		//Create VkSurface
		graphicsPipeline = std::make_unique<GraphicsPipeline>(instance->instance, instance->window);

		//Create physical and logical device
		devices = std::make_unique<VulkanDevices>();
		devices->pickPhysicalDevice(instance->instance, graphicsPipeline->surface);
		devices->createLogicalDevice(instance->validationLayers, graphicsPipeline->surface);

		//Create the components of the graphics pipeline
		VkPhysicalDevice physicalDevice = devices->getPhysicalDevice();
		VkDevice logicalDevice = devices->getLogicalDevice();
		  //Create swapchain
		  graphicsPipeline->createSwapchain(logicalDevice, physicalDevice, instance->window);
		  //Create image views
		  graphicsPipeline->createImageViews(logicalDevice);

	};

	void mainLoop() {
		while (!glfwWindowShouldClose(instance->window)) {
			glfwPollEvents();
		}
	};

	void cleanup() {
		VkDevice logicalDevice = devices->getLogicalDevice();
		//Cleans up VkSurface
		graphicsPipeline->cleanup(logicalDevice, instance->instance);
	};
};

int main() {
	Application app;

	try {
		app.run();
	}
	catch (const std::exception& e) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
};