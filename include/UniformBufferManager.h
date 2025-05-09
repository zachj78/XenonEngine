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
	UniformBufferManager(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkDescriptorSetLayout pipeline_descriptorSetLayout,std::shared_ptr<BufferManager> bufferManager) : descManager_logicalDevice(logicalDevice), descManager_physicalDevice(physicalDevice), descriptorSetLayout(pipeline_descriptorSetLayout), descManager_bufferManager(bufferManager) {};

	void createUniformBuffers();

	void updateUniformBuffer(uint32_t currentImage, VkExtent2D swapchainExtent);

	void createDescriptorPool();

	void createDescriptorSets();

	void cleanup();

	//Getter functions
	std::vector<VkDescriptorSet> getDescriptorSets() const { return descriptorSets; };

	//Setter functions

private: 
	//Injected Buffer Manager Utility 
	std::shared_ptr<BufferManager> descManager_bufferManager; 

	VkDevice descManager_logicalDevice;
	VkPhysicalDevice descManager_physicalDevice;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets; 

	std::unordered_map<std::string, std::shared_ptr<Buffer>> uniformBuffers;
	std::vector<void*> uniformBuffer_ptrs;
};

#endif