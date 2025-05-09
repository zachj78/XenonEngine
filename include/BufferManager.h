#pragma once
#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include "config.h"
#include "MeshManager.h"
#include "Buffer.h"

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
		VkDeviceSize bufferSize,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		std::optional<std::vector<Vertex>> vertices = std::nullopt,
		std::optional<std::vector<uint32_t>> indices = std::nullopt
	);
	
	void BufferManager::copyBuffer(std::shared_ptr<Buffer> srcBuffer, std::shared_ptr<Buffer> dstBuffer, VkDeviceSize size);
	
	std::shared_ptr<Buffer> getBuffer(const std::string& name);

private:
	std::unordered_map<std::string, std::shared_ptr<Buffer>> buffers;

	VkDevice bufferManager_logicalDevice;
	VkPhysicalDevice bufferManager_physicalDevice;
	VkCommandPool bufferManager_commandPool;
	VkQueue bufferManager_graphicsQueue;
};

#endif