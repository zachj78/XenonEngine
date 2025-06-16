#include "../include/Core/Swapchain.h"

#include "../include/Managers/Image.h"
#include "../include/Managers/ShaderLoader.h"
#include "../include/Managers/BufferManager.h"

#include "../include/Utils/MemoryUtils.h"

// -- Logger functions -- 
static void logSwapchain(uint32_t imageCount, VkExtent2D swapExtent, VkFormat surfaceFormat, VkPresentModeKHR presentMode, VkSwapchainCreateInfoKHR swapchainInfo) {
	printf("Swapchain created successfully!\n");
	printf("  Image count: %u\n", imageCount);
	printf("  Resolution: %ux%u\n", swapExtent.width, swapExtent.height);

	printf("  Format: %d\n", surfaceFormat);

	printf("  Present mode: ");
	switch (presentMode) {
	case VK_PRESENT_MODE_IMMEDIATE_KHR:     printf("Immediate\n"); break;
	case VK_PRESENT_MODE_MAILBOX_KHR:       printf("Mailbox\n"); break;
	case VK_PRESENT_MODE_FIFO_KHR:          printf("FIFO (V-Sync)\n"); break;
	case VK_PRESENT_MODE_FIFO_RELAXED_KHR:  printf("FIFO Relaxed\n"); break;
	default:                                 printf("Unknown (%d)\n", presentMode); break;
	}

	printf("  Sharing mode: %s\n",
		static_cast<int>(swapchainInfo.imageSharingMode) == static_cast<int>(VK_SHARING_MODE_CONCURRENT)
		? "Concurrent"
		: "Exclusive");
};

void RenderTargeter::cleanup(bool isLastCleanup) {
	std::cout << "    Destroying `Swapchain` " << std::endl;

	VkDevice logicalDevice = swpch_devices->getLogicalDevice();
	VkInstance instance = swpch_instance->getInstance();
	VkSurfaceKHR surface = swpch_instance->getSurface();

	//Clean up depth image
	if (renderTarget.depthImage != nullptr) {
		renderTarget.depthImage->cleanup();
	}
	
	if (!renderTarget.isSwapchain) {
		for (size_t i = 0; i < renderTarget.offscreenFramebuffers.size(); i++) {
			vkDestroyFramebuffer(logicalDevice, renderTarget.offscreenFramebuffers[i], nullptr);
		}
	}

	for (size_t i = 0; i < renderTarget.offscreenFramebuffers.size(); i++) {
		vkDestroyFramebuffer(logicalDevice, renderTarget.mainFramebuffers[i], nullptr);
	}

	for (size_t i = 0; i < renderTarget.imageViews.size(); i++) {
		vkDestroyImageView(logicalDevice, renderTarget.imageViews[i], nullptr);
	}

	if (swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(logicalDevice, swapchain, nullptr);
		swapchain = VK_NULL_HANDLE; 
	}

	//RESET RENDER TARGET
	renderTarget.isSwapchain = false; 
	renderTarget.offscreenFramebuffers.clear();
	renderTarget.imageViews.clear();
	renderTarget.depthImage = nullptr;

	if (!renderTarget.isSwapchain && isLastCleanup) {
		vkDestroyRenderPass(logicalDevice, offscreenPass, nullptr);
		offscreenPass = VK_NULL_HANDLE;
	}

	//ONLY DELETE RENDERPASS IF ON FINAL CLEANUP
	if (isLastCleanup) {
		vkDestroyRenderPass(logicalDevice, mainPass, nullptr);
		mainPass = VK_NULL_HANDLE;
	}
};

void RenderTargeter::createDepthImage() {
	std::cout << "[RenderTargeter::createDepthImage] entered" << std::endl;
	VkDevice logicalDevice = swpch_devices->getLogicalDevice();
	VkPhysicalDevice physicalDevice = swpch_devices->getPhysicalDevice();

	std::shared_ptr<Image> depthImage = std::make_shared<Image>(logicalDevice, physicalDevice);

	depthImage->createDepthImage(renderTarget.extent);
	renderTarget.depthImage = std::move(depthImage);
	std::cout << "[RenderTargeter::createDepthImage] exited" << std::endl;
}

void RenderTargeter::createSwapchainResources() {
	std::cout << "=== START createSwapchainResources ===" << std::endl;

	VkDevice logicalDevice = swpch_devices->getLogicalDevice();
	VkPhysicalDevice physicalDevice = swpch_devices->getPhysicalDevice();
	GLFWwindow* window = swpch_instance->getWindowPtr();
	VkSurfaceKHR surface = swpch_instance->getSurface();

	std::cout << "[LOG] Querying swapchain support..." << std::endl;
	SwapchainSupportDetails supportDetails = querySwapchainSupport(physicalDevice, surface);

	std::cout << "[LOG] Present modes available: " << supportDetails.presentModes.size() << std::endl;

	VkPresentModeKHR presentMode = chooseSwapPresentMode(supportDetails.presentModes);
	std::cout << "[LOG] Chosen present mode: " << presentMode << std::endl;

	uint32_t imageCount = supportDetails.capabilities.minImageCount + 1;
	if (supportDetails.capabilities.maxImageCount > 0 && imageCount > supportDetails.capabilities.maxImageCount) {
		imageCount = supportDetails.capabilities.maxImageCount;
	}
	std::cout << "[LOG] Final image count: " << imageCount << std::endl;

	renderTarget.isSwapchain = true;

	VkSwapchainCreateInfoKHR swapchainInfo{};
	swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainInfo.surface = surface;
	swapchainInfo.minImageCount = imageCount;

	//DEBUG
	std::cout << "[RemderTargeter::createSwapchainResources] EXTENT : [h:" << renderTarget.extent.height << " w: " << renderTarget.extent.width << "] \n"
		<< "[RemderTargeter::createSwapchainResources] SURFACE FORMAT: " << renderTarget.format << std::endl;
	swapchainInfo.imageFormat = renderTarget.format;
	swapchainInfo.imageExtent = renderTarget.extent;
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
	std::cout << "[LOG] Graphics Family: " << indices.graphicsFamily.value()
		<< " | Present Family: " << indices.presentFamily.value() << std::endl;

	if (indices.graphicsFamily != indices.presentFamily) {
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainInfo.queueFamilyIndexCount = 2;
		swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainInfo.queueFamilyIndexCount = 0;
		swapchainInfo.pQueueFamilyIndices = nullptr;
	}

	swapchainInfo.preTransform = supportDetails.capabilities.currentTransform;
	swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainInfo.presentMode = presentMode;
	swapchainInfo.clipped = VK_TRUE;

	VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE;
	if (swapchain != VK_NULL_HANDLE) {
		oldSwapchain = swapchain;
		std::cout << "[LOG] Replacing old swapchain..." << std::endl;
	}

	swapchainInfo.oldSwapchain = oldSwapchain;

	std::cout << "[LOG] Creating swapchain..." << std::endl;
	if (vkCreateSwapchainKHR(logicalDevice, &swapchainInfo, nullptr, &swapchain) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create the swapchain");
	}
	std::cout << "[LOG] Swapchain created: " << swapchain << std::endl;

	if (oldSwapchain != VK_NULL_HANDLE) {
		std::cout << "[LOG] Destroying old swapchain: " << oldSwapchain << std::endl;
		vkDestroySwapchainKHR(logicalDevice, oldSwapchain, nullptr);
	}

	std::cout << "[LOG] Getting swapchain images..." << std::endl;
	VkResult getImagesResult = vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, nullptr);
	if (getImagesResult != VK_SUCCESS || imageCount == 0) {
		throw std::runtime_error("Failed to get swapchain images");
	}

	std::cout << "[LOG] Swapchain image count: " << imageCount << std::endl;

	renderTarget.images.resize(imageCount);
	vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, renderTarget.images.data());

	std::cout << "[LOG] Creating image views..." << std::endl;
	renderTarget.imageViews.resize(imageCount);
	for (size_t i = 0; i < renderTarget.images.size(); i++) {
		std::cout << "[LOG] Creating image view for image index: " << i << std::endl;
		VkImageViewCreateInfo viewCreateInfo{};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.image = renderTarget.images[i];
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = renderTarget.format;
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.levelCount = 1;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = 1;

		VkResult viewResult = vkCreateImageView(logicalDevice, &viewCreateInfo, nullptr, &renderTarget.imageViews[i]);
		if (viewResult != VK_SUCCESS) {
			std::cerr << "[ERROR] Failed to create image view at index " << i << std::endl;
			throw std::runtime_error("Failed to create image views");
		}
	}

	std::cout << "[DEBUG] renderTarget.images.size(): " << renderTarget.images.size() << std::endl;
	std::cout << "[DEBUG] renderTarget.imageViews.size(): " << renderTarget.imageViews.size() << std::endl;

	std::cout << "=== END createSwapchainResources ===" << std::endl;
}

void RenderTargeter::createFramebuffers(VkRenderPass renderPass, bool usingOffscreenPass) {
	std::cout << "RenderTargeter[createFramebuffers] entered" << std::endl;
	VkDevice logicalDevice = swpch_devices->getLogicalDevice();

	if (usingOffscreenPass) {
		std::cout << "[RenderTargeter::createFramebuffers] creating offscreen frame buffers" << std::endl;

		//Resize the framebuffers to hold each image view
		renderTarget.offscreenFramebuffers.resize(renderTarget.imageViews.size());
	} else {
		std::cout << "[RenderTargeter::createFramebuffers] creating main frame buffers" << std::endl;

		renderTarget.mainFramebuffers.resize(renderTarget.imageViews.size());

		std::cout << "Main framebuffer vec size: " << renderTarget.mainFramebuffers.size() << std::endl;
		std::cout << "Addrs: \n"
			<< "[0]: " << renderTarget.mainFramebuffers[0] << "\n"
			<< "[1]: " << renderTarget.mainFramebuffers[1] << "\n"
			<< "[2]: " << renderTarget.mainFramebuffers[2] << std::endl;
	}

	std::cout << "[RenderTargeter::createFramebuffers] Depth image view handle: " << renderTarget.depthImage->getImageDetails().imageView << std::endl;
	
	//now iterate through image views and create a framebuffer for each
	for (size_t i = 0; i < renderTarget.imageViews.size(); i++) {
		std::cout << "[RenderTargeter::createFramebuffers] Color image view handle: "  << i  << " : " << renderTarget.imageViews[i] << std::endl;

		std::array<VkImageView, 2> attachments = {
			renderTarget.imageViews[i],
			renderTarget.depthImage->getImageDetails().imageView
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = renderTarget.extent.width;
		framebufferInfo.height = renderTarget.extent.height;
		framebufferInfo.layers = 1;

		if (usingOffscreenPass) {
			if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &renderTarget.offscreenFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create main framebuffer!");
			}
		} else {
			if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &renderTarget.mainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create offscreen framebuffer!");
			}
		}
	}
}

void RenderTargeter::transitionTargetImageLayout(
	VkCommandBuffer cmdBuffer,
	VkImage image,
	VkFormat format,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	VkImageAspectFlags aspectMask,
	uint32_t mipLevels,
	uint32_t baseMipLevel,
	uint32_t baseArrayLayer,
	uint32_t layerCount
) {
	std::cout << "TRANSITION FROM :: " << oldLayout << " to :: " << newLayout << std::endl;
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = aspectMask;
	barrier.subresourceRange.baseMipLevel = baseMipLevel;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = baseArrayLayer;
	barrier.subresourceRange.layerCount = layerCount;

	// Choose source and destination access masks based on layouts
	VkPipelineStageFlags srcStage;
	VkPipelineStageFlags dstStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else {
		throw std::invalid_argument("Unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		cmdBuffer,
		srcStage,
		dstStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
}


VkSurfaceFormatKHR RenderTargeter::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	};

	return availableFormats[0];
};
VkPresentModeKHR RenderTargeter::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	};

	// if triple buffering is not available then just use double buffering
	return VK_PRESENT_MODE_FIFO_KHR;
};

VkExtent2D RenderTargeter::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow *window) {
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	} else {
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