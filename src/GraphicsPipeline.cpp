#include "../include/GraphicsPipeline.h"

// -- Logger functions -- 
void logSwapchain(uint32_t imageCount, VkExtent2D swapExtent, VkSurfaceFormatKHR surfaceFormat, VkPresentModeKHR presentMode, VkSwapchainCreateInfoKHR swapchainInfo) {
	printf("Swapchain created successfully!\n");
	printf("  Image count: %u\n", imageCount);
	printf("  Resolution: %ux%u\n", swapExtent.width, swapExtent.height);

	printf("  Format: %d\n", surfaceFormat.format);
	printf("  Color space: %d\n", surfaceFormat.colorSpace);

	printf("  Present mode: ");
	switch (presentMode) {
	case VK_PRESENT_MODE_IMMEDIATE_KHR:     printf("Immediate\n"); break;
	case VK_PRESENT_MODE_MAILBOX_KHR:       printf("Mailbox\n"); break;
	case VK_PRESENT_MODE_FIFO_KHR:          printf("FIFO (V-Sync)\n"); break;
	case VK_PRESENT_MODE_FIFO_RELAXED_KHR:  printf("FIFO Relaxed\n"); break;
	default:                                 printf("Unknown (%d)\n", presentMode); break;
	}

	printf("  Sharing mode: %s\n",
		swapchainInfo.imageSharingMode == VK_SHARING_MODE_CONCURRENT
		? "Concurrent"
		: "Exclusive");
}

// Creates a window surface
GraphicsPipeline::GraphicsPipeline(VkInstance &instance, GLFWwindow* window) {
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create a window surface");
	};
};

//Cleans up window surface, swapchain and swapchain image views
void GraphicsPipeline::cleanup(VkDevice logicalDevice, VkInstance &instance) {
	for (const auto& swapchainImageView : swapchainImageViews) {
		vkDestroyImageView(logicalDevice, swapchainImageView, nullptr);
	};

	vkDestroySwapchainKHR(logicalDevice, swapchain, nullptr);
	
	vkDestroySurfaceKHR(instance, surface, nullptr);
};

//These functions are responsible for creating the swapchain
// -> note: before swapchain is created extension supported must be checked in VulkanDevices
void GraphicsPipeline::createSwapchain(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, GLFWwindow* window) {
	SwapchainSupportDetails supportDetails = querySwapchainSupport(physicalDevice, surface);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(supportDetails.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(supportDetails.presentModes);
	VkExtent2D swapExtent = chooseSwapExtent(supportDetails.capabilities, window);
	
	//choose image count 
	// we add +1 so that the swapchain does not have to wait on the last image to be rendered to begin a new one
	uint32_t imageCount = supportDetails.capabilities.minImageCount + 1;

	if (supportDetails.capabilities.maxImageCount > 0 && imageCount > supportDetails.capabilities.maxImageCount) {
		imageCount = supportDetails.capabilities.maxImageCount;
	};

	VkSwapchainCreateInfoKHR swapchainInfo{};
	swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainInfo.surface = surface; 
	swapchainInfo.minImageCount = imageCount; 
	swapchainInfo.imageFormat = surfaceFormat.format;
	swapchainInfo.imageExtent = swapExtent;
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily) {
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainInfo.queueFamilyIndexCount = 2; 
		swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainInfo.queueFamilyIndexCount = 0;
		swapchainInfo.pQueueFamilyIndices = nullptr;
	};

	swapchainInfo.preTransform = supportDetails.capabilities.currentTransform; // No pre-transform
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // ignore alpha channel -> no mixing
	swapchainInfo.presentMode = presentMode;
	swapchainInfo.clipped = VK_TRUE;

	swapchainInfo.oldSwapchain = VK_NULL_HANDLE; // this will later be used to rebuild the swapchain in the event it becomes invalid

	if (vkCreateSwapchainKHR(logicalDevice, &swapchainInfo, nullptr, &swapchain) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create the swapchain");
	}
	else {
		logSwapchain(imageCount, swapExtent, surfaceFormat, presentMode, swapchainInfo);
	}


	// now we retreive the actual swapchain images
	vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, nullptr);
	swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, swapchainImages.data());

};

VkSurfaceFormatKHR GraphicsPipeline::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	};

	return availableFormats[0];
};

VkPresentModeKHR GraphicsPipeline::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	};

	// if triple buffering is not available then just use double buffering
	return VK_PRESENT_MODE_FIFO_KHR;
};

VkExtent2D GraphicsPipeline::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow *window) {
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		//actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

void GraphicsPipeline::createImageViews(VkDevice logicalDevice) {
	swapchainImageViews.resize(swapchainImages.size());

	for (size_t i = 0; i < swapchainImages.size(); i++) {
		VkImageViewCreateInfo viewCreateInfo{};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.image = swapchainImages[i];
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = swapchainImageFormat;

		// -> subresourceRange field describes the image's purpose
		// -> we specify we are using these images for color targets
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.levelCount = 1;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = 1;
	
		if (vkCreateImageView(logicalDevice, &viewCreateInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create image views");
		};
	};
}