#include "../include/GraphicsPipeline.h"

// -- Logger functions -- 
static void logSwapchain(uint32_t imageCount, VkExtent2D swapExtent, VkSurfaceFormatKHR surfaceFormat, VkPresentModeKHR presentMode, VkSwapchainCreateInfoKHR swapchainInfo) {
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

GraphicsPipeline::GraphicsPipeline(VkInstance &instance, GLFWwindow* window) {
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create a window surface");
	};
};

void GraphicsPipeline::cleanup(VkDevice logicalDevice, VkInstance &instance) {
	for (auto framebuffer : swapchainFramebuffers) {
		vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
	};
	
	vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
	vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
	
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

	swapchainImageFormat = surfaceFormat.format;
	swapchainExtent = swapExtent;
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

void GraphicsPipeline::createRenderPass(VkDevice logicalDevice) {
	std::cout << "Entering createRenderPass()" << std::endl;

	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapchainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	//No stencil buffer currently, dont store data
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	//Initial and final layout of the swapchain image
	// -> in this case final layout is VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0; //refers the the index of this attachment
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create render pass");
	};
};

void GraphicsPipeline::createGraphicsPipeline(VkDevice logicalDevice) {
	std::cout << "Entering createGraphicsPipeline()" << std::endl;

    //Create a shader loader class to load vert and frag shader
	shaderLoader = std::make_unique<ShaderLoader>();

	auto vertShaderCode = shaderLoader->readShaderFile("shaders/vert.spv");
	auto fragShaderCode = shaderLoader->readShaderFile("shaders/frag.spv");

	VkShaderModule vertShaderModule = shaderLoader->createShaderModule(logicalDevice, vertShaderCode); 
	VkShaderModule fragShaderModule = shaderLoader->createShaderModule(logicalDevice, fragShaderCode);

	//Now we assign the shaders to the pipeline using PipelineShaderStageCreateInfo
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{}; 
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule; 
	vertShaderStageInfo.pName = "main"; 

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule; 
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	//Create dynamic state create info struct 
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	//Specifies vertex data to be drawn
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr; 
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr; 

	//Specifies how input vertices should be assembled
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; 
	inputAssembly.primitiveRestartEnable = VK_FALSE; 

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1; 
	viewportState.scissorCount = 1;

	//Specifies how rasterization will work
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.rasterizerDiscardEnable = VK_FALSE; 
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f; 
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	//configues multi-sampling -> kept disabled for now
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE; 
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; 

	//for 3d shit create a depth buffer and set it up here later

	//specifies how color blending will work
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	//finally create the pipeline layout 
	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	if (vkCreatePipelineLayout(logicalDevice, &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	};

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2; 
	pipelineInfo.pStages = shaderStages;
	//reference all the stages of the pipeline
	pipelineInfo.pVertexInputState = &vertexInputInfo; 
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending; 
	pipelineInfo.pDepthStencilState = nullptr; 
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout; 
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0; //specify subpass index where pipeline will be used

	VkResult result = vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);

	std::cout << "Finished creating graphics pipeline -> result : " << result << std::endl;

	//Delete shader modules
	vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
};

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

void GraphicsPipeline::createSwapFramebuffers(VkDevice logicalDevice) {
	//Resize the framebuffers to hold each image view
	swapchainFramebuffers.resize(swapchainImageViews.size());

	//now iterate through image views and create a framebuffer for each
	for (size_t i = 0; i < swapchainImageViews.size(); i++) {
		VkImageView attachments[] = {
			swapchainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapchainExtent.width;
		framebufferInfo.height = swapchainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &swapchainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create framebuffer");
		}
	}
}