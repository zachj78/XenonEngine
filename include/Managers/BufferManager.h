#pragma once
#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include "Utils/config.h"
#include "Managers/Buffer.h"
#include "Managers/Vertex.h"

//Forward declarations
class GraphicsPipeline; 
class Buffer; 
class Vertex; 

class BufferManager {
public:
	BufferManager(
		VkDevice logicalDevice,
		VkPhysicalDevice physicalDevice,
		VkQueue graphicsQueue
	);

	void cleanup();

	void createBuffer(
		BufferType type,
		const std::string& name,
		VkDeviceSize bufferSize,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		std::optional<std::vector<Vertex>> vertices = std::nullopt,
		std::optional<std::vector<uint32_t>> indices = std::nullopt
	);
	
	VkCommandBuffer beginOneTimeCommands(VkCommandPool commandPool);

	void endOneTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool commandPool);

	void copyBuffer(std::shared_ptr<Buffer> srcBuffer, std::shared_ptr<Buffer> dstBuffer, VkDeviceSize size, VkCommandPool commandPool);

	std::shared_ptr<Buffer> getBuffer(const std::string& name);

	void removeBufferByName(const std::string name);

private:
	std::unordered_map<std::string, std::shared_ptr<Buffer>> buffers;

	VkDevice bufferManager_logicalDevice;
	VkPhysicalDevice bufferManager_physicalDevice;
	std::shared_ptr<GraphicsPipeline> bufferManager_graphicsPipeline;
	VkQueue bufferManager_graphicsQueue;
};

#endif