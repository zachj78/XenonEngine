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

//Forward declarations
class DescriptorManager; 
class MeshManager; 
class DescriptorManaager; 
class BufferManager; 
class Buffer;

class GraphicsPipeline {
public:
	// Constructor
	GraphicsPipeline(std::shared_ptr<VulkanInstance> instance, 
		std::shared_ptr<Devices> devices, 
		std::shared_ptr<Swapchain> swapchain
	)
		: instance(instance), devices(devices), swapchain(swapchain) {
	}

	// Manually track whether window has been resized
	bool framebufferResized = false;

	// Pipeline setup
	void createGraphicsPipeline(std::array<VkDescriptorSetLayout, 3> descriptorSetLayouts);
	void createRenderPass();
	void createCommandPool();
	void createCommandBuffer();
	void createSyncObjects();

	// Main frame draw function
	void drawFrame(GLFWwindow* window, bool framebufferResized,
		std::shared_ptr<DescriptorManager> descriptorManager,
		std::shared_ptr<BufferManager> bufferManager,
		std::shared_ptr<MeshManager> meshManager,
		std::shared_ptr<SwapchainRecreater> swapchainRecreater
	);

	void recordCommandBuffer(VkCommandBuffer commandBuffer,
		uint32_t imageIndex,
		std::shared_ptr<DescriptorManager> descriptorManager,
		std::shared_ptr<BufferManager> bufferManager,
		std::shared_ptr<MeshManager> meshManager
	);

	// Cleanup
	void cleanup();

	// Getters
	VkPipeline getGraphicsPipeline() { return graphicsPipeline; };
	VkPipelineLayout getPipelineLayout() { return pipelineLayout; };
	VkRenderPass getRenderPass() { return renderPass; };
	std::vector<VkCommandBuffer> getCommandBuffers() { return commandBuffers; };
	VkCommandPool getCommandPool() { return commandPool; };

	uint32_t getCurrentFrame() { return currentFrame; };

private:
	// Injected vulkan core component classes
	std::shared_ptr<Devices> devices = nullptr;
	std::shared_ptr<Swapchain> swapchain = nullptr;
	std::shared_ptr<VulkanInstance> instance = nullptr; 

	//Injected resource managers
	std::shared_ptr<DescriptorManager> descriptorManager;

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