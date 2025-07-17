#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

//Config and main helpers/universal structs
#include "Utils/config.h"
#include "Utils/cstm_types.h"

//Vulkan Components
#include "Core/Swapchain.h"
#include "Core/VulkanDevices.h"

//These are utility classes used within this class
#include "Managers/ShaderLoader.h"
#include "Managers/SwapchainRecreater.h"
#include "Managers/MeshManager.h"

//Forward declarations
class DescriptorManager; 
class DescriptorManaager; 
class BufferManager; 
class Buffer;
class GUI; 
class Mesh;

class GraphicsPipeline {
public:
	// Constructor
	GraphicsPipeline(std::shared_ptr<VulkanInstance> instance, 
		std::shared_ptr<Devices> devices)
		: instance(instance), devices(devices){
	}

	// Manually track whether window has been resized
	bool framebufferResized = false;

	// Pipeline setup
	void createGraphicsPipeline(std::shared_ptr<RenderTargeter> renderTargeter, std::array<VkDescriptorSetLayout, 3> descriptorSetLayouts);
	void createCommandPool();
	void createCommandBuffer();
	void createSyncObjects(uint32_t imagesPerFrame);

	// === Main frame draw functions ===
	//Drawing w/ Swapchain
	void drawSwapchain(
		GLFWwindow* window,
		bool framebufferResized,
		const std::shared_ptr<DescriptorManager>& descriptorManager,
		const std::shared_ptr<BufferManager>& bufferManager,
		const std::shared_ptr<MeshManager>& meshManager,
		const std::shared_ptr<SwapchainRecreater>& swapchainRecreater,
		const std::shared_ptr<GUI>& gui,
		const std::shared_ptr<RenderTargeter>& renderTargeter
	);
	//Drawing w/ full offscreen target set up
	void drawOffscreen(
		GLFWwindow* window,
		bool framebufferResized,
		const std::shared_ptr<DescriptorManager>& descriptorManager,
		const std::shared_ptr<BufferManager>& bufferManager,
		const std::shared_ptr<MeshManager>& meshManager,
		const std::shared_ptr<SwapchainRecreater>& swapchainRecreater,
		const std::shared_ptr<GUI>& gui,
		const std::shared_ptr<RenderTargeter>& renderTargeter
	);

	// REFACTOR THIS FUNCTION INTO THE ABOVE FUNCTIONS,
	// [NOTE]: DRAW OFFSCREEN WILL BE LARGELY INCOMPLETE FOR NOW

	//Draw with ONLY swapchain
	void recordFullDraw(
		VkCommandBuffer commandBuffer,
		uint32_t imageIndex,
		const std::shared_ptr<DescriptorManager>& descriptorManager,
		const std::shared_ptr<BufferManager>& bufferManager,
		const std::shared_ptr<MeshManager>& meshManager,
		const std::shared_ptr<GUI>& gui,
		const std::shared_ptr<RenderTargeter>& renderTargeter
	);

	//Draw with offscreen target
	void recordOffscreenDraw(VkCommandBuffer commandBuffer,
		uint32_t imageIndex,
		std::shared_ptr<DescriptorManager> descriptorManager,
		std::shared_ptr<BufferManager> bufferManager,
		std::shared_ptr<MeshManager> meshManager,
		std::shared_ptr<GUI> gui,
		std::shared_ptr<RenderTargeter> renderTargeter);

	void drawPrimitive(
		VkCommandBuffer commandBuffer,
		const std::shared_ptr<BufferManager>& bufferManager,
		const std::shared_ptr<Primitive> primitivePtr,
		bool usePushConstant); 


	// Cleanup
	void cleanup();

	// Getters
	VkPipeline getGraphicsPipeline() { return graphicsPipeline; };
	VkPipelineLayout getPipelineLayout() { return pipelineLayout; };
	std::vector<VkCommandBuffer> getCommandBuffers() { return commandBuffers; };
	VkCommandPool getCommandPool() { return commandPool; };

	uint32_t getCurrentFrame() { return currentFrame; };

private:
	// Injected vulkan core component classes
	std::shared_ptr<Devices> devices = nullptr;
	std::shared_ptr<RenderTargeter> renderTargeter = nullptr; 
	std::shared_ptr<VulkanInstance> instance = nullptr; 

	//Injected resource managers
	std::shared_ptr<DescriptorManager> descriptorManager;

	uint32_t currentFrame = 0;

	// Graphics Pipeline
	VkPipelineLayout pipelineLayout;

	VkPipeline graphicsPipeline;

	std::unordered_map<PipelineKey, VkPipeline> pipelineByKey; 

	std::shared_ptr<ShaderLoader> shaderLoader;
	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	// Command system
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

	// Sync objects
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
};

#endif