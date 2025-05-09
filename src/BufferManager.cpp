#include "../include/BufferManager.h"
#include "../include/UniformBufferManager.h"

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


// == Buffer Operation functions == 
void BufferManager::createBuffer(
	BufferType type,
	const std::string& name,
	VkDeviceSize bufferSize, 
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	std::optional<std::vector<Vertex>> vertices,
	std::optional<std::vector<uint32_t>> indices
) 
{
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

void BufferManager::copyBuffer(std::shared_ptr<Buffer> srcBuffer, std::shared_ptr<Buffer> dstBuffer, VkDeviceSize size) {
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

// == Retreival functions == 
std::shared_ptr<Buffer> BufferManager::getBuffer(const std::string& name) {
	auto it = buffers.find(name);
	if (it == buffers.end()) {
		throw std::runtime_error("Buffer" + name + " not found BufferManager::getBuffer()");
	}

	return it->second;
};