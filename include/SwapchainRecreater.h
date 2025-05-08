#ifndef SWAPCHAIN_RECREATER_H
#define SWAPCHAIN_RECREATER_H
#include "config.h"

class SwapchainRecreater {
public:
	void setCallbacks(
		std::function<void()> cleanupFunc,
		std::function<void()> recreateSwapchain,
		std::function<void()> recreateImageViews,
		std::function<void()> recreateFramebuffers
	);

	void recreateSwapchain(VkDevice logicalDevice, GLFWwindow* window);

private:
	std::function<void()> cleanupCallback;
	std::function<void()> recreateSwapchainCallback;
	std::function<void()> recreateImageViewsCallback;
	std::function<void()> recreateFramebuffersCallback;
};

#endif