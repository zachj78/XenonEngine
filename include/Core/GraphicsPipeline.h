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

struct PipelineComponents {
	VkPipeline graphicsPipeline;
	VkPipelineLayout pipelineLayout;
};

class GraphicsPipeline {
public:
	// Constructor
	GraphicsPipeline(std::shared_ptr<VulkanInstance> instance,
		std::shared_ptr<Devices> devices)
		: instance(instance), devices(devices) {
	}

	// Manually track whether window has been resized
	bool framebufferResized = false;

	// Pipeline setup
	//[LEAVE OFF POINT : CHECK THAT YOU CALL THIS NEW FUNCTION INSTEAD OF THE OLD CREATE GRAPHICS PIPELINE FROM RENDERER.CPP, THEN MAYBE TEST?]
	// AFTER THAT TRY TO REFACTOR SOME BASIC STUFF, GO OVER OWNERSHIP OF CUSTOM DEFINED SHIT, DONT OVER ALLOCATE SHARED_PTRS
	void createGraphicsPipelines(
		std::shared_ptr<RenderTargeter> renderTargeter,
		std::array<VkDescriptorSetLayout, 3> descriptorSetLayouts,
		const std::unordered_set<PipelineKey>& pipelineKeys
	);

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
		const std::shared_ptr<DescriptorManager>& descriptorManager,
		const std::shared_ptr<BufferManager>& bufferManager,
		const std::shared_ptr<MeshManager>& meshManager,
		const std::shared_ptr<GUI>& gui,
		const std::shared_ptr<RenderTargeter>& renderTargeter
	);

	void drawPrimitive(
		VkCommandBuffer commandBuffer,
		const std::shared_ptr<BufferManager>& bufferManager,
		const std::shared_ptr<Primitive>& primitivePtr,
		const VkPipelineLayout& pipelineLayout,
		bool usePushConstant
	);


	// Cleanup
	void cleanup();

	// Getters
	const PipelineComponents const& getPipelineComponentByKey(const PipelineKey& key) {
		auto& it = pipelinesByKey.find(key);

		if (it != pipelinesByKey.end()) {
			return *(it->second);
		}
		else {
			throw std::runtime_error("Failed to find PipelineComponent By Key");
		}
	};

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

	// Graphics Pipelines
	std::unordered_map<PipelineKey, std::unique_ptr<PipelineComponents>> pipelinesByKey;

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