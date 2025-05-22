#pragma once
#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include "config.h"
#include "Buffer.h"
#include "Vertex.h"

//Forward declarations
class GraphicsPipeline; 
class Buffer; 
class Vertex; 

class BufferManager {
public:
	BufferManager(
		VkDevice logicalDevice,
		VkPhysicalDevice physicalDevice,
		std::shared_ptr<GraphicsPipeline> graphicsPipeline,
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
	
	VkCommandBuffer beginOneTimeCommands();

	void endOneTimeCommands(VkCommandBuffer commandBuffer);

	void copyBuffer(std::shared_ptr<Buffer> srcBuffer, std::shared_ptr<Buffer> dstBuffer, VkDeviceSize size);

	std::shared_ptr<Buffer> getBuffer(const std::string& name);
	VkCommandPool getCommandPool();

	void removeBufferByName(const std::string name);

private:
	std::unordered_map<std::string, std::shared_ptr<Buffer>> buffers;

	VkDevice bufferManager_logicalDevice;
	VkPhysicalDevice bufferManager_physicalDevice;
	std::shared_ptr<GraphicsPipeline> bufferManager_graphicsPipeline;
	VkQueue bufferManager_graphicsQueue;
};

#endif