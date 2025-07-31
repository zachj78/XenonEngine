#include "../include/Core/GraphicsPipeline.h"
#include "../include/Managers/BufferManager.h"
#include "../include/Managers/Buffer.h"
#include "../include/Utils/MemoryUtils.h"
#include "../include/Managers/DescriptorManager.h"
#include "../include/Managers/MeshManager.h"
#include "../include/System_Components/GUI.h"

void GraphicsPipeline::cleanup() {
	std::cout << "    Destroying `GraphicsPipeline` " << std::endl;

	VkDevice logicalDevice = devices->getLogicalDevice();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(logicalDevice, imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(logicalDevice, inFlightFences[i], nullptr);
	}
	vkDestroyCommandPool(logicalDevice, commandPool, nullptr);

	for (auto& pipelineComponents : pipelinesByKey) {
		VkPipeline& graphicsPipeline = pipelineComponents.second->graphicsPipeline;
		VkPipelineLayout& pipelineLayout = pipelineComponents.second->pipelineLayout;

		vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
	}
};

//[PLAN] -> MODIFY THIS CLASS TO CHECK THE LOADED PRIMITIVES PIPELINE KEYS
// IT SHOULD THEN CREATE THE MATCHING PIPELINES AS WELL AS A PIPELINE FOR 
//  1 - ImGui Render(figure that out later)
//  2 - infinite Grid shader
// Get normal pipeline working first - remember to keep comitting to git so if you fuck something up you can go back to the last working version

//updated createGraphicsPipeline(s)
//create multiple layouts then for each layout make a pipeline, allocate these in a vector
// replace layout, pipeline -> and their getter functions; change references to this function appropropriately - mark so you can find the refactors later
// std::vector<VkPipelineLayout> layouts; 
// std::vector<VkPipeline> pipelines; //pipelines bound to specific primitives, to be bound and rebound based on primitive at draw
// VkPipeline gridPipeline; //pipelien for infinite grid shader, can be bound before primnitive pipeline
void GraphicsPipeline::createGraphicsPipelines(
	std::shared_ptr<RenderTargeter> renderTargeter,
	std::array<VkDescriptorSetLayout, 3> descriptorSetLayouts,
	const std::unordered_set<PipelineKey>& pipelineKeys
) {
	std::cout << "[GraphicsPipeline::createGraphicsPipelines] entereed\n";
	std::cout << "Creating " << pipelineKeys.size() << " pipelines\n";

	VkDevice logicalDevice = devices->getLogicalDevice();

	//Create infinite grid pipeline here 

	for (size_t i = 0; i < pipelineKeys.size(); i++) {

		PipelineComponents pipelineComponent = { VK_NULL_HANDLE, VK_NULL_HANDLE };

		//[DEBUG - log] 
		std::cout << "Creating pipeline with key << " << i << "/" << pipelineKeys.size() << std::endl;

		auto key = std::next(pipelineKeys.begin(), i);

		if (key != pipelineKeys.end()) {
			std::cout << "key addr : " << key->packed << "\n";
		}
		else {
			std::cerr << "Failed to find pipeline key at index : " << i << "\n";
		}

		//for now load the standard shaders

		//Create a shader loader class to load vert and frag shader
		shaderLoader = std::make_shared<ShaderLoader>();

		auto vertShaderCode = shaderLoader->readShaderFile("resources/shaders/vert.spv");
		std::vector<char, std::allocator<char>> fragShaderCode;

		if (devices->getDeviceCaps().supportsBindless) {
			fragShaderCode = shaderLoader->readShaderFile("resources/shaders/frag.spv");
		}
		else {
			fragShaderCode = shaderLoader->readShaderFile("resources/shaders/frag_traditional.spv");
		}

		std::cout << "Loaded vertex shader, size: " << vertShaderCode.size() << std::endl;
		std::cout << "Loaded fragment shader, size: " << fragShaderCode.size() << std::endl;

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

		//Get per-vertex binding and attribute descriptions
		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescription = Vertex::getAttributeDescriptions();

		//Specifies vertex data to be drawn
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

		//Specifies how input vertices should be assembled
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

		//add in other primitives based on gLTF topology types
		switch (key->topology) {
		case 4:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssembly.primitiveRestartEnable = VK_FALSE;
			break;
		}

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

		if (key->cullMode == 1) {
			rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		}
		else if (key->cullMode == 2) {
			rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
		}
		else {
			rasterizer.cullMode = VK_CULL_MODE_NONE;
		}

		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		//configues multi-sampling -> kept disabled for now
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = (key->depthTest != 0) ? VK_TRUE : VK_FALSE;
		depthStencil.depthWriteEnable = (key->depthWrite != 0) ? VK_TRUE : VK_FALSE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

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

		//Specify per-mesh index passed as push constant
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(int);

		//finally create the pipeline layout 
		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		layoutInfo.pSetLayouts = descriptorSetLayouts.data();

		layoutInfo.pushConstantRangeCount = 1;
		layoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(logicalDevice, &layoutInfo, nullptr, &pipelineComponent.pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create pipeline layout");
		};

		std::cout << "[DEBUG] : RENDER PASS HANDLE: " << renderTargeter->getMainPass();

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		//reference all the stages of the pipeline
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = pipelineComponent.pipelineLayout;
		pipelineInfo.renderPass = renderTargeter->getMainPass();
		pipelineInfo.subpass = 0; //specify subpass index where pipeline will be used
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		VkResult result = vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipelineComponent.graphicsPipeline);

		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create graphics pipeline: error: " + std::to_string(result));
		};

		std::cout << "Finished creating graphics pipeline -> result : " << result << std::endl;

		pipelinesByKey[*key] = std::make_unique<PipelineComponents>(pipelineComponent);

		//Delete shader modules
		vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
		vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
	}
}

void GraphicsPipeline::createCommandPool() {
	//Define variables used to create command pool
	VkDevice logicalDevice = devices->getLogicalDevice();
	VkPhysicalDevice physicalDevice = devices->getPhysicalDevice();
	VkSurfaceKHR surface = instance->getSurface();

	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create command pool");
	};
};

void GraphicsPipeline::createCommandBuffer() {
	std::cout << "creating command buffers" << std::endl;

	VkDevice logicalDevice = devices->getLogicalDevice();

	//create a command buffer for each frame
	commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command buffers");
	};
}

void GraphicsPipeline::createSyncObjects(uint32_t imagesPerFrame) {
	VkDevice logicalDevice = devices->getLogicalDevice();

	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(imagesPerFrame);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	//Signal fence immediately, otherwise first frame will never draw
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < imagesPerFrame; i++) {
		if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create semaphores");
		}
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create fence/image available semaphore");
		}
	}
}

void GraphicsPipeline::drawOffscreen(
	GLFWwindow* window,
	bool framebufferResized,
	const std::shared_ptr<DescriptorManager>& descriptorManager,
	const std::shared_ptr<BufferManager>& bufferManager,
	const std::shared_ptr<MeshManager>& meshManager,
	const std::shared_ptr<SwapchainRecreater>& swapchainRecreater,
	const std::shared_ptr<GUI>& gui,
	const std::shared_ptr<RenderTargeter>& renderTargeter
) {
	VkDevice logicalDevice = devices->getLogicalDevice();
	RenderTarget& renderTarget = renderTargeter->getRenderTarget();
	bool isSwapchain = renderTarget.isSwapchain;
	VkExtent2D extent = renderTarget.extent;

	// [DEBUG] CHECK LAYOUTS AND OFFSCREEN FRAMEBUFFERS FOR OFFSCREEN DRAW
	std::cout << "offscreen framebuffer size: " << renderTarget.offscreenFramebuffers.size() << "\n" <<
		"layouts [0]:" << renderTarget.currentLayouts[0] << " [1]: " << renderTarget.currentLayouts[0] << "\n" <<
		"main framebuffer size: " << renderTarget.mainFramebuffers.size() << std::endl;

	std::cout << "[GraphicsPipeline::drawOffscreen] Waiting on inFlightFence[" << currentFrame << "]" << std::endl;

	// Wait for frame fence
	vkWaitForFences(logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	//[DEBUG]
	std::cout << "\n=== BEGIN FRAME " << currentFrame << " ===" << std::endl;
	std::cout << "Framebuffer resized: " << framebufferResized << std::endl;
	std::cout << "RenderTarget extent: " << extent.width << "x" << extent.height << std::endl;

	//Aquire next image
	uint32_t imageIndex = 0;

	if (renderTarget.images.size() == 0) {
		std::cout << "[Warning] renderTarget.images.size() is 0!" << std::endl;
	}
	else {
		//Cycle between both images
		imageIndex = renderTarget.imageIndex;
		renderTarget.imageIndex = (renderTarget.imageIndex + 1) % renderTarget.images.size();

		std::cout << "[GraphicsPipeline::drawOffscreen] current imageIndex =" << imageIndex << std::endl;
		std::cout << "[GraphicsPipeline::drawOffscreen] next imageIndex =" << renderTarget.imageIndex << std::endl;
	}

	std::cout << "[GraphicsPipeline::drawOffscreen] Reset inFlightFence fr{" << currentFrame << "}" << std::endl;

	// Reset fence & command buffer
	vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);
	vkResetCommandBuffer(commandBuffers[currentFrame], 0);

	// Record commands
	recordOffscreenDraw(commandBuffers[currentFrame], imageIndex, descriptorManager, bufferManager, meshManager, gui, renderTargeter);

	// Submit commands
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	// std::cout << "[Sync] Waiting on imageAvailableSemaphores[" << currentFrame << "]" << std::endl;
	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(devices->getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw command buffer");
	}

	// NO PRESENTATION AFTER QUEUE SUBMISSION

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	std::cout << "=== END FRAME " << currentFrame << " ===\n" << std::endl;
}

void GraphicsPipeline::drawSwapchain(GLFWwindow* window,
	bool framebufferResized,
	const std::shared_ptr<DescriptorManager>& descriptorManager,
	const std::shared_ptr<BufferManager>& bufferManager,
	const std::shared_ptr<MeshManager>& meshManager,
	const std::shared_ptr<SwapchainRecreater>& swapchainRecreater,
	const std::shared_ptr<GUI>& gui,
	const std::shared_ptr<RenderTargeter>& renderTargeter) {

	std::cout << "[GraphicsPipeline::drawSwapchain] entered" << std::endl;

	VkDevice logicalDevice = devices->getLogicalDevice();

	//Render Target info
	RenderTarget& renderTarget = renderTargeter->getRenderTarget();
	VkExtent2D extent = renderTarget.extent;

	// Wait for frame fence
	std::cout << "[GraphicsPipeline::drawSwapchain] -fr{" << currentFrame << "} Waiting on in-flight fence" << std::endl;
	vkWaitForFences(logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	//Aquire next image
	uint32_t imageIndex = 0;

	VkResult result = vkAcquireNextImageKHR(
		logicalDevice,
		renderTargeter->getSwapchain(),
		UINT64_MAX,
		imageAvailableSemaphores[currentFrame],
		VK_NULL_HANDLE,
		&imageIndex
	);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		swapchainRecreater->recreateSwapchain(logicalDevice, window);
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("Failed to acquire swap chain image");
	}

	// Reset fence & command buffer
	std::cout << "[GraphicsPipeline::drawSwaphchain] Reset inFlightFence -fr:{" << currentFrame << "}" << std::endl;
	vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);
	vkResetCommandBuffer(commandBuffers[currentFrame], 0);

	recordFullDraw(commandBuffers[currentFrame], imageIndex, descriptorManager, bufferManager, meshManager, gui, renderTargeter);

	// Submit commands
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	// std::cout << "[Sync] Waiting on imageAvailableSemaphores[" << currentFrame << "]" << std::endl;
	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[imageIndex] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(devices->getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw command buffer");
	}

	// Present Image

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderFinishedSemaphores[imageIndex];
	VkSwapchainKHR swapchains[] = { renderTargeter->getSwapchain() };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &imageIndex;

	VkResult presentResult = vkQueuePresentKHR(devices->getPresentQueue(), &presentInfo);

	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
		swapchainRecreater->recreateSwapchain(logicalDevice, window);
	}
	else if (presentResult != VK_SUCCESS) {
		throw std::runtime_error("Failed to present swapchain image");
	}

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	std::cout << "=== END FRAME " << currentFrame << " ===\n" << std::endl;
}

//For in game
void GraphicsPipeline::recordFullDraw(
	VkCommandBuffer commandBuffer,
	uint32_t imageIndex,
	const std::shared_ptr<DescriptorManager>& descriptorManager,
	const std::shared_ptr<BufferManager>& bufferManager,
	const std::shared_ptr<MeshManager>& meshManager,
	const std::shared_ptr<GUI>& gui,
	const std::shared_ptr<RenderTargeter>& renderTargeter
) {
	std::cout << "[CmdBuf] Recording 'full-draw' commandBuffer for imageIndex: " << imageIndex << std::endl;

	RenderTarget& renderTarget = renderTargeter->getRenderTarget();
	VkExtent2D extent = renderTarget.extent;

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("Failed to begin recording command buffer");
	}

	// === Main Render Pass ===
	{
		//gui->beginFrame(currentFrame);
		//gui->endFrame();

		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = renderTargeter->getMainPass();
		renderPassBeginInfo.framebuffer = renderTarget.mainFramebuffers[imageIndex];
		renderPassBeginInfo.renderArea = { {0, 0}, extent };

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassBeginInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		//Run infinite grid shader pipeline here

		//Bind pipeline based on pipeline key
		for (auto& pipelineKey : meshManager->getPipelineKeys()) {
			VkPipelineLayout& pipelineLayout = pipelinesByKey[pipelineKey]->pipelineLayout; 
			VkPipeline &graphicsPipeline = pipelinesByKey[pipelineKey]->graphicsPipeline;

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
		
			VkViewport viewport{};
			viewport.width = static_cast<float>(extent.width);
			viewport.height = static_cast<float>(extent.height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

			VkRect2D scissor{ {0, 0}, extent };
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

			// === Descriptor Sets Binding ===
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
				&descriptorManager->getDescriptorSets()[currentFrame], 0, nullptr);

			//Update camera per-frame
			descriptorManager->updateUniformBuffer(currentFrame, renderTargeter->getRenderTarget().extent);

			//Bind mesh transform descriptor sets
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1,
				&meshManager->getSSBODescriptorSets()[currentFrame], 0, nullptr);

			// == Draw Primitives == 
			const auto& primitives = meshManager->getPrimitiveByPipelineKey();
			const std::vector<std::shared_ptr<Primitive>>& primsPerPipeline = primitives.at(pipelineKey);

			// === Draw Meshes ===
			if (devices->getDeviceCaps().supportsDescriptorIndexing) {
				std::cout << "USING INDEXING\n"
					<< "material sets: " << meshManager->getMaterialDescriptorSets().size() << "\n";
				VkDescriptorSet bindlessMatSet = meshManager->getMaterialDescriptorSets()[currentFrame];

				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1,
					&bindlessMatSet, 0, nullptr);

				// Draws all primitives within the same pipeline key
				for (const auto& primitive : primsPerPipeline) {
					drawPrimitive(commandBuffer, bufferManager, primitive, pipelineLayout, true);
				};
			} else {
				std::cout << "NO INDEXING\n";
				std::cout << "model matrices count before draw: " << meshManager->getAllModelMatrices().size() << "\n";

				for (const auto& primitive : primsPerPipeline) {
					VkDescriptorSet materialSet = primitive->getMaterial()->getDescriptorSets()[currentFrame];

					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1,
						&materialSet, 0, nullptr);

					drawPrimitive(commandBuffer, bufferManager, primitive, pipelineLayout, true);
				};
			}
		}

		//gui->record(commandBuffer);

		vkCmdEndRenderPass(commandBuffer);
	}

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to record command buffer");
	}
}

////For offscreen editor view
void GraphicsPipeline::recordOffscreenDraw(VkCommandBuffer commandBuffer,
	uint32_t imageIndex,
	const std::shared_ptr<DescriptorManager>& descriptorManager,
	const std::shared_ptr<BufferManager>& bufferManager,
	const std::shared_ptr<MeshManager>& meshManager,
	const std::shared_ptr<GUI>& gui,
	const std::shared_ptr<RenderTargeter>& renderTargeter
) {
	//
	//	RenderTarget& renderTarget = renderTargeter->getRenderTarget();
	//	VkExtent2D extent = renderTarget.extent;
	//
	//	std::cout << "[CmdBuf] Recording 'offscreen-draw' commandBuffer for imageIndex: \n" << imageIndex << std::endl;
	//	std::cout << "[Offscreen] Current layout at frame start: " << renderTarget.currentLayouts[imageIndex] << "\n" << std::endl;
	//
	//	VkCommandBufferBeginInfo beginInfo{};
	//	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	//
	//	gui->beginFrame(currentFrame);
	//	gui->endFrame();
	//
	//	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
	//		throw std::runtime_error("Failed to begin recording command buffer");
	//	}
	//
	//	std::cout << "LAYOUT BEFORE GUI PASS: " << renderTarget.currentLayouts[imageIndex];
	//
	//	// === GUI Render Pass ===
	//	{
	//		VkRenderPassBeginInfo guiRenderBeginInfo{};
	//		guiRenderBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	//		guiRenderBeginInfo.renderPass = renderTargeter->getOffscreenPass();
	//		guiRenderBeginInfo.framebuffer = renderTarget.offscreenFramebuffers[imageIndex];
	//		guiRenderBeginInfo.renderArea = { {0, 0}, extent };
	//
	//		std::array<VkClearValue, 2> guiClearValues{};
	//		guiClearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	//		guiClearValues[1].depthStencil = { 1.0f, 0 };
	//
	//		guiRenderBeginInfo.clearValueCount = static_cast<uint32_t>(guiClearValues.size());
	//		guiRenderBeginInfo.pClearValues = guiClearValues.data();
	//
	//		vkCmdBeginRenderPass(commandBuffer, &guiRenderBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	//
	//		vkCmdEndRenderPass(commandBuffer);
	//		
	//		//SET NEW IMAGE LAYOUT
	//		renderTarget.currentLayouts[imageIndex] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	//	}
	//	
	//	// === Layout Transition for GUI Input ===
	//	//if (renderTarget.currentLayouts[imageIndex] != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
	//	//	std::cout << "[Transition] Layout[" << imageIndex << "] = "
	//	//		<< renderTarget.currentLayouts[imageIndex]
	//	//		<< " => VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL" << std::endl;
	//
	//	//	renderTargeter->transitionTargetImageLayout(
	//	//		commandBuffer,
	//	//		renderTarget.images[imageIndex],
	//	//		renderTarget.format,
	//	//		renderTarget.currentLayouts[imageIndex],
	//	//		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	//	//		VK_IMAGE_ASPECT_COLOR_BIT
	//	//	);
	//	//	renderTarget.currentLayouts[imageIndex] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	//	//}
	//	//else {
	//	//	std::cout << "First frame, image is already in COLOR_ATTACHMENT_OPTIMAL" << std::endl;
	//	//}
	//
	//	// === Layout Transition for GUI Output ===
	//	//if (renderTarget.currentLayouts[imageIndex] != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
	//	//	std::cout << "[Transition] Layout[" << imageIndex << "] = "
	//	//		<< renderTarget.currentLayouts[imageIndex]
	//	//		<< " => VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL" << std::endl;
	//
	//	//	renderTargeter->transitionTargetImageLayout(
	//	//		commandBuffer,
	//	//		renderTarget.images[imageIndex],
	//	//		renderTarget.format,
	//	//		renderTarget.currentLayouts[imageIndex],
	//	//		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	//	//		VK_IMAGE_ASPECT_COLOR_BIT
	//	//	);
	//	//	renderTarget.currentLayouts[imageIndex] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	//	//}
	//	//else {
	//	//	std::cout << "ALREADY SET TO LAYOUT :: SHADER_READ_ONLY_OPTIMAL" << std::endl;
	//	//}
	//
	//
	//	// === Main Render Pass ===
	//	{
	//		VkRenderPassBeginInfo renderPassBeginInfo{};
	//		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	//		renderPassBeginInfo.renderPass = renderTargeter->getMainPass();
	//		renderPassBeginInfo.framebuffer = renderTarget.mainFramebuffers[imageIndex];
	//		renderPassBeginInfo.renderArea = { {0, 0}, extent };
	//
	//		std::array<VkClearValue, 2> clearValues{};
	//		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	//		clearValues[1].depthStencil = { 1.0f, 0 };
	//		renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	//		renderPassBeginInfo.pClearValues = clearValues.data();
	//
	//		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	//		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
	//
	//		VkViewport viewport{};
	//		viewport.width = static_cast<float>(extent.width);
	//		viewport.height = static_cast<float>(extent.height);
	//		viewport.minDepth = 0.0f;
	//		viewport.maxDepth = 1.0f;
	//		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	//
	//		VkRect2D scissor{ {0, 0}, extent };
	//		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	//
	//		// === Descriptor Sets Binding ===
	//
	//		// CAMERA UBO BINDING
	//		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
	//			&descriptorManager->getDescriptorSets()[currentFrame], 0, nullptr);
	//
	//		// == Draw Primitives ==
	//		meshManager->getPrimitiveByPipelineKey(); 
	//
	//		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1,
	//			&meshManager->getSSBODescriptorSets()[currentFrame], 0, nullptr);
	//
	//		// === Draw Meshes ===
	//		const auto& meshes = meshManager->getAllMeshes();
	//
	//		if (devices->getDeviceCaps().supportsDescriptorIndexing) {
	//			VkDescriptorSet bindlessMatSet = meshManager->getMaterialDescriptorSets()[currentFrame];
	//
	//			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1,
	//				&bindlessMatSet, 0, nullptr);
	//
	//			for (const auto& [meshName, meshPtr] : meshes) {
	//				drawMesh(commandBuffer, bufferManager, meshName, meshPtr, true);
	//			}
	//		}
	//		else {
	//			for (const auto& [meshName, meshPtr] : meshes) {
	//				VkDescriptorSet materialSet = meshPtr->getMaterial()->getDescriptorSets()[currentFrame];
	//
	//				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1,
	//					&materialSet, 0, nullptr);
	//
	//				drawMesh(commandBuffer, bufferManager, meshName, meshPtr, true);
	//			}
	//		}
	//
	//		vkCmdEndRenderPass(commandBuffer);
	//	}
	//
	//	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
	//		throw std::runtime_error("Failed to record command buffer");
	//	}
	//	else {
	//		std::cout << "[CmdBuf] Finished recording command buffer!" << std::endl;
	//	}
}

void GraphicsPipeline::drawPrimitive(
	VkCommandBuffer commandBuffer,
	const std::shared_ptr<BufferManager>& bufferManager,
	const std::shared_ptr<Primitive>& primitivePtr,
	const VkPipelineLayout& pipelineLayout, 
	bool usePushConstant
) {
	int primitiveIndex = primitivePtr->getPrimitiveIndex();
	int meshIndex = primitivePtr->getParentMeshIndex();

	std::cout << "Drawing primitive : prim" << primitiveIndex << "\n with mesh index " << meshIndex << std::endl;

	std::shared_ptr<Buffer> vbuf = bufferManager->getBuffer("vbuf" + std::to_string(primitiveIndex));
	std::shared_ptr<Buffer> ibuf = bufferManager->getBuffer("ibuf" + std::to_string(primitiveIndex));

	if (!vbuf || !ibuf) {
		std::cerr << "Missing buffers for prim: " << primitiveIndex << std::endl;
		return;
	}
	else {
		std::cout << "vbuf addr: " << vbuf->getHandle() << "\n"
			<< "ibuf addr: " << ibuf->getHandle() << std::endl;
	}

	VkBuffer vertexBuffer = vbuf->getHandle();
	VkBuffer indexBuffer = ibuf->getHandle();
	auto indices = ibuf->getData<uint32_t>();

	VkBuffer vertexBuffers[] = { vertexBuffer };
	VkDeviceSize offsets[] = { 0 };

	if (usePushConstant) {
		std::cout << "Mesh index to push: " << meshIndex << std::endl;

		vkCmdPushConstants(commandBuffer, pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
			sizeof(int), &meshIndex);
	}

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}