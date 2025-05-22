#ifndef SWAPCHAIN_RECREATER_H
#define SWAPCHAIN_RECREATER_H
#include "config.h"

class SwapchainRecreater {
public:
	void setCallbacks(
		std::function<void()> cleanupSwapchain,
		std::function<void()> recreateSwapchain,
		std::function<void()> recreateImageViews,
		std::function<void()> recreateFramebuffers,
		std::function<void()> cleanupDepthResources,
		std::function<void()> recreateDepthResources
	);

	void recreateSwapchain(VkDevice logicalDevice, GLFWwindow* window);

private:
	std::function<void()> cleanupSwapchainCallback;
 	std::function<void()> recreateSwapchainCallback;
	std::function<void()> recreateImageViewsCallback;
	std::function<void()> recreateFramebuffersCallback;
	std::function<void()> cleanupDepthResourcesCallback;
	std::function<void()> recreateDepthResourcesCallback;
};

#endif