#pragma once
#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include "config.h"
#include "MeshManager.h"
#include "Buffer.h"

//CREATES A UNIFORM BUFFER -> CALLED FROM UNIFORMBUFFERMANAGER
namespace BufferUtils {

	std::shared_ptr<BaseBuffer> createUniformBuffer(BufferType type,
		std::string name,
		VkDeviceSize size,
		VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
		VkDevice logicalDevice, VkPhysicalDevice physicalDevice
	);

}

class BufferManager {
public:
	BufferManager(
		VkDevice logicalDevice,
		VkPhysicalDevice physicalDevice,
		VkCommandPool commandPool,
		VkQueue graphicsQueue
	);

	//CREATES VERTEX, INDEX AND STAGING BUFFERS
	void createBuffer(
		BufferType type,
		const std::string& name,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		std::optional<std::vector<Vertex>> vertices = std::nullopt,
		std::optional<std::vector<uint32_t>> indices = std::nullopt
	);

	//CREATES AN EMPTY BUFFER -> GET RID OF TYPE PARAM
	void BufferManager::createRawBuffer(
		BufferType type,
		std::string name,
		VkDeviceSize size,
		VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	
	void BufferManager::copyBuffer(Buffer* srcBuffer, Buffer* dstBuffer, VkDeviceSize srcBufferSize); 
	
	Buffer* getBuffer(const std::string& name);

private:
	std::unordered_map<std::string, std::shared_ptr<Buffer>> buffers;

	VkDevice bufferManager_logicalDevice;
	VkPhysicalDevice bufferManager_physicalDevice;
	VkCommandPool bufferManager_commandPool;
	VkQueue bufferManager_graphicsQueue;
};

#endif