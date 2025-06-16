#pragma once
#ifndef SWAPCHAIN_RECREATER_H
#define SWAPCHAIN_RECREATER_H

#include "Utils/config.h"

class SwapchainRecreater {
public:
	void setCallbacks(
		std::function<void()> cleanup,
		std::function<void()> getFramebufferDetails,
		std::function<void()> recreateDepthResources,
		std::function<void()> recreateTarget,
		std::function<void()> recreateFramebuffers
	);

	void recreateSwapchain(VkDevice logicalDevice, GLFWwindow* window);

private:
	std::function<void()> cleanupCallback; // Cleans up swapchain or offscreen target
	std::function<void()> framebufferDetailsCallback; // Sets appropriate render target extent and format
 	std::function<void()> recreateCallback; // Recreates swapchain or offscreen target
	std::function<void()> recreateFramebuffersCallback; 
	std::function<void()> recreateDepthResourcesCallback;
};

#endif