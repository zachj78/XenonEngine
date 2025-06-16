#include "../include/Managers/SwapchainRecreater.h"

void SwapchainRecreater::setCallbacks (
	std::function<void()> cleanup,
	std::function<void()> getFramebufferDetails,
	std::function<void()> recreateDepthResources,
	std::function<void()> recreateTarget,
	std::function<void()> recreateFramebuffers
) {
    cleanupCallback = cleanup;
	framebufferDetailsCallback = getFramebufferDetails;
	recreateCallback = recreateTarget;
	recreateDepthResourcesCallback = recreateDepthResources;
	recreateFramebuffersCallback = recreateFramebuffers;
}

void SwapchainRecreater::recreateSwapchain(VkDevice logicalDevice, GLFWwindow* window) {
	std::cout << "[SwapchainRecreater::recreateSwapchain] entered" << std::endl;
	int width, height = 0;

	glfwGetFramebufferSize(window, &width, &height);

	while (width == 0 || height == 0) {
		glfwWaitEvents();
		glfwGetFramebufferSize(window, &width, &height);
	}

	vkDeviceWaitIdle(logicalDevice);

	cleanupCallback();
	framebufferDetailsCallback();
	recreateDepthResourcesCallback();
	recreateCallback();
	recreateFramebuffersCallback();

	std::cout << "[SwapchainRecreater::recreateSwapchain] exited" << std::endl;
}
