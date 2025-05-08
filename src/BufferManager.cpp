#include "../include/BufferManager.h"
#include "../include/UniformBufferManager.h"

namespace BufferUtils {
	std::shared_ptr<BaseBuffer> createUniformBuffer(BufferType type,
		std::string name,
		VkDeviceSize size,
		VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
		VkDevice logicalDevice, VkPhysicalDevice physicalDevice
	) {
		std::cout << "Creating uniform buffer" << std::endl;

		auto newBuffer = std::make_shared<BaseBuffer>(type, name);
		if (newBuffer.get() == nullptr) {
			throw std::runtime_error("Failed to construct Base Buffer");
		}

		newBuffer->allocateAndBindBuffer(
			logicalDevice,
			physicalDevice,
			size,
			usage,
			properties);

		return newBuffer;
	};


}

BufferManager::BufferManager(
	VkDevice logicalDevice,
	VkPhysicalDevice physicalDevice,
	VkCommandPool commandPool,
	VkQueue graphicsQueue
) {
	bufferManager_logicalDevice = logicalDevice;
	bufferManager_physicalDevice = physicalDevice;
	bufferManager_commandPool = commandPool;
	bufferManager_graphicsQueue = graphicsQueue;
};

void BufferManager::createBuffer(
	BufferType type,
	const std::string& name,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	std::optional<std::vector<Vertex>> vertices,
	std::optional<std::vector<uint32_t>> indices
) 
{
	std::cout << "Creating buffer : " << name << std::endl;
	VkDeviceSize bufferSize; 

	//Get buffer size, depending on type
	if (type == BufferType::VERTEX || type == BufferType::VERTEX_STAGING) {
		if (vertices.has_value()) {
			bufferSize = sizeof((*vertices)[0]) * vertices->size();
			std::cout << "passed v data: " << vertices->size() << std::endl;
		} else {
			throw std::runtime_error("Error: No vertex data passed during vertex buffer creation -> BufferManager::createBuffer");
		};
	} else if (type == BufferType::INDEX || type == BufferType::INDEX_STAGING) {
		if (indices.has_value()) {
			std::cout << "passed i data: " << indices->size() << std::endl;
			bufferSize = sizeof(uint32_t) * indices->size();
			std::cout << "i data size: " << bufferSize << std::endl;
		} else {
			throw std::runtime_error("Error: No index data passed duing index buffer creation -> BufferManager::createBuffer");
		};
	}

	//Create a new buffer instance
	auto newBuffer = std::make_shared<Buffer>(
		type,
		name,
		bufferSize,
		bufferManager_logicalDevice,
		bufferManager_physicalDevice,
		vertices, 
		indices
	);

	newBuffer->createBuffer(bufferManager_logicalDevice, bufferManager_physicalDevice, usage, properties, bufferSize);

	buffers[name] = std::move(newBuffer);
};

void BufferManager::createRawBuffer(
	BufferType type, 
	std::string name,
	VkDeviceSize size,
	VkBufferUsageFlags usage, VkMemoryPropertyFlags properties
) {

	std::cout << "Creating raw buffer" << std::endl;

	auto newBuffer = std::make_shared<BaseBuffer>(type, name);
	if (newBuffer.get() == nullptr) {
		throw std::runtime_error("Failed to construct Base Buffer");
	}

	newBuffer->allocateAndBindBuffer(
		bufferManager_logicalDevice,
		bufferManager_physicalDevice,
		size,
		usage,
		properties);
};


Buffer* BufferManager::getBuffer(const std::string& name) {
	auto it = buffers.find(name);
	if (it == buffers.end()) {
		throw std::runtime_error("Buffer" + name + " not found BufferManager::getBuffer()");
	}

	return it->second.get();
};

void BufferManager::copyBuffer(Buffer* srcBuffer, Buffer* dstBuffer, VkDeviceSize size) {
	std::cout << "Copy staging data into v/i buffer" << std::endl;

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = bufferManager_commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(bufferManager_logicalDevice, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	//Copy buffer to dstBuffer
	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer->getHandle(), dstBuffer->getHandle(), 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(bufferManager_graphicsQueue, 1, &submitInfo, nullptr);
	vkQueueWaitIdle(bufferManager_graphicsQueue);

	vkFreeCommandBuffers(bufferManager_logicalDevice, bufferManager_commandPool, 1, &commandBuffer);
};