#pragma once
#ifndef GUI_H
#define GUI_H

#include "Utils/config.h"
#include "Utils/MemoryUtils.h"

//Forward definitions
class GraphicsPipeline; 

class GUI {
public: 
	GUI() {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	}

	//MAYBE FLATTEN INTO STRUCT `LINK_CONTEXT`? [FLAGGED]
	void linkToApp(
		bool usingSwapchain, 
		GLFWwindow* window,
		VkInstance instance,
		VkDevice logicalDevice,
		VkPhysicalDevice physicalDevice,
		uint32_t graphicsQueueIndex,
		VkQueue graphicsQueue,
		uint32_t minImageCount,
		uint32_t imageCount,
		std::vector<VkImageView> targetImageViews,
		VkRenderPass renderPass,
		VkSampler targetSampler = nullptr
	);

	void beginFrame(uint32_t currentFrame /* VkSampler sampler, VkImageView imageView*/);
	void endFrame();
	void record(VkCommandBuffer commandBuffer);

private: 
	//GUI STATE
	bool showLeftPanel = false; 
	bool showAssetManager = false;

	//Descriptor pool
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout offscreenLayout;

	std::vector<ImTextureID> offscreenTextureIDs;
};

#endif