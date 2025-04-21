#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#include "config.h"
#include "cstm_types.h"
#include "ShaderLoader.h"

class GraphicsPipeline {
public: 
	//Variables
	VkSurfaceKHR surface; 

	//Constructor - creates window surface
	GraphicsPipeline(VkInstance &instance, GLFWwindow* window);

	//Create the swapchain and its components
	void createSwapchain(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, GLFWwindow* window);
	 VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	 VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	 VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilties, GLFWwindow* window);
	 void createImageViews(VkDevice logicalDevice);

	// This functions sets up the main graphics pipeline
	void createGraphicsPipeline(VkDevice logicalDevice);

	//This function creates the render pass 
	void createRenderPass(VkDevice logicalDevice);

	//This function creates framebuffers
	void createSwapFramebuffers(VkDevice logicalDevice);

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
	std::vector<VkFramebuffer> swapchainFramebuffers; 

	//Graphics Pipeline Variables
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	 //Loads shader from w/in graphics pipeline
	 std::unique_ptr<ShaderLoader> shaderLoader;
	 //dynamic states used w/in graphics pipeline
	 std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	 };

	//Render pass variables
	VkRenderPass renderPass;
};

#endif