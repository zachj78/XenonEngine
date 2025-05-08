#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H

#include "config.h"
#include "cstm_types.h"
#include "ShaderLoader.h"
#include "VulkanInstance.h"
#include "VulkanDevices.h"

class Swapchain {
public:
	Swapchain(std::shared_ptr<VulkanInstance> instance, std::shared_ptr<Devices> devices)
		: swpch_instance(instance), swpch_devices(devices) {
	};

	// ==MAIN FUNCTIONS==
	//Create the swapchain and its components
	void createSwapchain();
	void createImageViews();
	void createSwapFramebuffers(VkRenderPass renderpass);

	//GET RID OF SURFACE CLEAN UP AND ADD TO INSTANCE <<<----
	void cleanup();

	//Getter functions
	//For the swapchain variables
	VkSwapchainKHR getSwapchain() { return swapchain; };
	std::vector<VkImage> getSwapchainImages() { return swapchainImages; };
	VkFormat getSwapchainImageFormat() { return swapchainImageFormat; };
	VkExtent2D getSwapchainExtent() { return swapchainExtent; };
	std::vector<VkFramebuffer> getSwapchainFramebuffers() { return swapchainFramebuffers; };


private:
	//Inject Vulkan Core Components
	std::shared_ptr<VulkanInstance> swpch_instance = nullptr; 
	std::shared_ptr<Devices> swpch_devices = nullptr; 

	//Swapchain variables
	VkSwapchainKHR swapchain;
	std::vector<VkImage> swapchainImages;
	VkFormat swapchainImageFormat; 
	VkExtent2D swapchainExtent; 
	std::vector<VkImageView> swapchainImageViews;
	std::vector<VkFramebuffer> swapchainFramebuffers; 

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilties, GLFWwindow* window);

	// ==HELPER FUNCTIONS==
};

#endif