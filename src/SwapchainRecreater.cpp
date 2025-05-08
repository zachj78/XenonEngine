#include "../include/SwapchainRecreater.h"

void SwapchainRecreater::setCallbacks(std::function<void()> cleanupFunc,
	std::function<void()> recreateSwapchain,
	std::function<void()> recreateImageViews,
	std::function<void()> recreateFramebuffers) {

	cleanupCallback = cleanupFunc;
	recreateSwapchainCallback = recreateSwapchain;
	recreateImageViewsCallback = recreateImageViews;
	recreateFramebuffersCallback = recreateFramebuffers;
}

void SwapchainRecreater::recreateSwapchain(VkDevice logicalDevice, GLFWwindow* window) {
	std::cout << "Recreating swapchain" << std::endl;
	int width, height = 0;

	glfwGetFramebufferSize(window, &width, &height);

	while (width == 0 || height == 0) {
		glfwWaitEvents();
		glfwGetFramebufferSize(window, &width, &height);
	}

	vkDeviceWaitIdle(logicalDevice);

	cleanupCallback();
	recreateSwapchainCallback();
	recreateImageViewsCallback();
	recreateFramebuffersCallback();
}
