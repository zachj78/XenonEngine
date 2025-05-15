#include "../include/BufferManager.h"
#include "../include/UniformBufferManager.h"
#include "../include/GraphicsPipeline.h"

BufferManager::BufferManager(
	VkDevice logicalDevice,
	VkPhysicalDevice physicalDevice,
	std::shared_ptr<GraphicsPipeline> graphicsPipeline,
	VkQueue graphicsQueue
) {
	bufferManager_logicalDevice = logicalDevice;
	bufferManager_physicalDevice = physicalDevice;
	bufferManager_graphicsPipeline = graphicsPipeline;
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

VkCommandBuffer BufferManager::beginOneTimeCommands() {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = getCommandPool();
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(bufferManager_logicalDevice, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
};

void BufferManager::endOneTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(bufferManager_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(bufferManager_graphicsQueue);

	vkFreeCommandBuffers(bufferManager_logicalDevice, getCommandPool(), 1, &commandBuffer);
};

//The way this function is used doesnt really follow the normal pattern of object retreival
// should probably make each Buffer copy its contents to a dst, should be called after buffer is retreived through manager
void BufferManager::copyBuffer(std::shared_ptr<Buffer> srcBuffer, std::shared_ptr<Buffer> dstBuffer, VkDeviceSize size) {
	std::cout << "Copy staging data into v/i buffer" << std::endl;
	VkCommandBuffer commandBuffer = beginOneTimeCommands();

	//Copy buffer to dstBuffer
	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer->getHandle(), dstBuffer->getHandle(), 1, &copyRegion);

	endOneTimeCommands(commandBuffer);
};

// == Deletion function == 
void BufferManager::removeBufferByName(const std::string name) {
	auto it = buffers.find(name);
	if (it != buffers.end()) {
		std::cout << "Removing buffer : [" << name << "]" << std::endl;
		buffers.erase(it);
	}
	else {
		std::cout << "Buffer [" << name << "] not found" << std::endl;
	};
};

// == Retreival functions == 
std::shared_ptr<Buffer> BufferManager::getBuffer(const std::string& name) {
	auto it = buffers.find(name);
	if (it == buffers.end()) {
		throw std::runtime_error("Buffer" + name + " not found BufferManager::getBuffer()");
	}

	return it->second;
};

VkCommandPool BufferManager::getCommandPool() { return bufferManager_graphicsPipeline->getCommandPool(); };