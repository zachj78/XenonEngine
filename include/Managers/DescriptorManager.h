#pragma once
#ifndef DESCRIPTOR_MANAGER_H
#define DESCRIPTOR_MANAGER_H

#include "Utils/config.h"
#include "Builders/DescriptorBuilder.h"

//Forward declarations
class BufferManager; 
class Buffer;
class Camera; 

struct UBO {
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec3 lightPos; 
	alignas(16) glm::vec3 lightColor; 
	alignas(16) glm::vec3 cameraPos; 
};

class DescriptorManager {
public:
	//RESEARCH PUSH CONSTANTS - THEY ARE MORE EFFICIENT FOR SMALL AMOUNTS OF DATA BEING PASSED TO SHADERS
	DescriptorManager(
		VkDevice logicalDevice, VkPhysicalDevice physicalDevice, 
		std::shared_ptr<BufferManager> bufferManager, 
		std::shared_ptr<Camera> camera
	) : descManager_logicalDevice(logicalDevice), 
	descManager_physicalDevice(physicalDevice), 
	descManager_bufferManager(bufferManager), 
	descManager_camera(camera) {};

	void createUniformBuffers();

	void updateUniformBuffer(uint32_t currentImage, VkExtent2D swapchainExtent);

	void createDescriptorPool(int meshCount, int materialCount);

	void createPerFrameDescriptors();

	void cleanup();

	//Getter functions
	std::vector<VkDescriptorSet> getDescriptorSets() const { return descriptorSets; };
	VkDescriptorPool getDescriptorPool() { return descriptorPool; };
	VkDescriptorSetLayout getDescriptorSetLayout() { return descriptorSetLayout; };

	//Setter functions

private: 
	//Injected Buffer Manager Utility 
	std::shared_ptr<BufferManager> descManager_bufferManager; 
	//Injected Core Components
	VkDevice descManager_logicalDevice;
	VkPhysicalDevice descManager_physicalDevice;

	//Camera
	std::shared_ptr<Camera> descManager_camera; 

	//Descriptor Info
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets; 

	std::vector<UniformBufferInfo> ubufInfo;
};

#endif