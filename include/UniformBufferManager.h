#pragma once
#ifndef UNIFORM_BUFFER_MANAGER_H
#define UNIFORM_BUFFER_MANAGER_H

#include "config.h"
#include "BufferManager.h"

struct UniformBufferObject {
	glm::mat4 model; 
	glm::mat4 view;
	glm::mat4 proj;
};

class UniformBufferManager {
public:
	//RESEARCH PUSH CONSTANTS - THEY ARE MORE EFFICIENT FOR SMALL AMOUNTS OF DATA BEING PASSED TO SHADERS
	UniformBufferManager(VkDevice logicalDevice, VkPhysicalDevice physicalDevice)
		: descManager_logicalDevice(logicalDevice), descManager_physicalDevice(physicalDevice) {};

	void createDescriptorSetLayout();

	void createUniformBuffers();

	void updateUniformBuffer(uint32_t currentImage, VkExtent2D swapchainExtent);

	void createDescriptorPool();

	void createDescriptorSet();

	void cleanup();

	//Getter functions
	std::vector<VkDescriptorSet> getDescriptorSets() const { return descriptorSets; };
	VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; };

	//Setter functions

private: 
	VkDevice descManager_logicalDevice;
	VkPhysicalDevice descManager_physicalDevice;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets; 

	std::unordered_map<std::string, std::shared_ptr<BaseBuffer>> uniformBuffers;
	std::vector<void*> uniformBuffer_ptrs;
};

#endif