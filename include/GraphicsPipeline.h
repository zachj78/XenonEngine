#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#include "config.h"
#include "cstm_types.h"

class GraphicsPipeline {
public: 
	//Variables
	VkSurfaceKHR surface; 

	//Constructor - creates window surface
	GraphicsPipeline(VkInstance &instance, GLFWwindow* window);

	void createSwapchain(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, GLFWwindow* window);

	//These functions are called from within createSwapchain
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilties, GLFWwindow* window);
	void createImageViews(VkDevice logicalDevice);
	//

	//This functions cleans up components fo the Graphics Pipeline:
	// -> surface, swapchain
	void cleanup(VkDevice logicalDevice, VkInstance &instance);

	//Getter functions
	//For the swapchain variables
	VkSwapchainKHR getSwapchain() { return swapchain; };
	std::vector<VkImage> getSwapchainImages() { return swapchainImages; };
	VkFormat getSwapchainImageFormat() { return swapchainImageFormat; };
	VkExtent2D getSwapchainExtent() { return swapchainExtent; };

private:
	//Swapchain variables
	VkSwapchainKHR swapchain;
	std::vector<VkImage> swapchainImages;
	VkFormat swapchainImageFormat; 
	VkExtent2D swapchainExtent; 
	std::vector<VkImageView> swapchainImageViews; 

	//Render pass variables
	
};

#endif