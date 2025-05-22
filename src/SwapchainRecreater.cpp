#include "../include/SwapchainRecreater.h"

void SwapchainRecreater::setCallbacks(
    std::function<void()> cleanupSwapchain,
    std::function<void()> recreateSwapchain,
    std::function<void()> recreateImageViews,
    std::function<void()> recreateFramebuffers,
	std::function<void()> cleanupDepthResources,
	std::function<void()> recreateDepthResources
) {
    cleanupSwapchainCallback = cleanupSwapchain;
	recreateSwapchainCallback = recreateSwapchain;
    recreateImageViewsCallback = recreateImageViews;
    recreateFramebuffersCallback = recreateFramebuffers;
	cleanupDepthResourcesCallback = cleanupDepthResources;
	recreateDepthResourcesCallback = recreateDepthResources;
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

	cleanupSwapchainCallback();
	cleanupDepthResourcesCallback();
	recreateSwapchainCallback();
	recreateImageViewsCallback();
	recreateDepthResourcesCallback();
	recreateFramebuffersCallback();
}
