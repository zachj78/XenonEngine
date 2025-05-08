#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

//Config and main helpers/universal structs
#include "config.h"
#include "cstm_types.h"

//Vulkan Components
#include "Swapchain.h"
#include "VulkanDevices.h"

//These are utility classes used within this class
#include "ShaderLoader.h"
#include "SwapchainRecreater.h"
#include "BufferManager.h"
#include "MeshManager.h"
#include "UniformBufferManager.h"

class GraphicsPipeline {
public:
	// Constructor
	GraphicsPipeline(std::shared_ptr<VulkanInstance> instance, std::shared_ptr<Devices> devices, std::shared_ptr<Swapchain> swapchain)
		: instance(instance), devices(devices), swapchain(swapchain) {
	}

	// Manually track whether window has been resized
	bool framebufferResized = false;

	// Pipeline setup
	void createGraphicsPipeline(std::shared_ptr<UniformBufferManager> uniformBufferManager);
	void createRenderPass();
	void createCommandPool();
	void createCommandBuffer();
	void createSyncObjects();

	// Main frame draw function
	void drawFrame(GLFWwindow* window, bool framebufferResized, 
		BufferManager* bufferManager, 
		SwapchainRecreater* swapchainRecreater,
		std::shared_ptr<UniformBufferManager> uniformBufferManager
	);

	void recordCommandBuffer(VkCommandBuffer commandBuffer,
		uint32_t imageIndex,
		BufferManager* bufferManager,
		VkDescriptorSet descriptorSet);

	// Cleanup
	void cleanup(VkInstance& instance);

	// Getters
	VkPipeline getGraphicsPipeline() { return graphicsPipeline; }
	VkPipelineLayout getPipelineLayout() { return pipelineLayout; }
	VkRenderPass getRenderPass() { return renderPass; }
	std::vector<VkCommandBuffer> getCommandBuffers() { return commandBuffers; }
	VkCommandPool getCommandPool() { return commandPool; }

private:
	// Injected vulkan core component classes
	std::shared_ptr<Devices> devices = nullptr;
	std::shared_ptr<Swapchain> swapchain = nullptr;
	std::shared_ptr<VulkanInstance> instance = nullptr; 

	uint32_t currentFrame = 0;

	// Graphics Pipeline
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	std::shared_ptr<ShaderLoader> shaderLoader;
	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	// Render pass
	VkRenderPass renderPass;

	// Command system
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

	// Sync objects
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
};

#endif