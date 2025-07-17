#include "../include/Managers/BufferManager.h"
#include "../include/Managers/DescriptorManager.h"
#include "../include/Core/GraphicsPipeline.h"

BufferManager::BufferManager(
	VkDevice logicalDevice,
	VkPhysicalDevice physicalDevice,
	VkQueue graphicsQueue
) {
	std::cout << "Creating [bufferManager]: " << std::endl;
	bufferManager_logicalDevice = logicalDevice;
	bufferManager_physicalDevice = physicalDevice;
	bufferManager_graphicsQueue = graphicsQueue;

	std::cout << "       with logical device: " << bufferManager_logicalDevice << std::endl;
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
	std::cout << "Creating buffer :[" << name << "]" << std::endl;

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

VkCommandBuffer BufferManager::beginOneTimeCommands(VkCommandPool commandPool) {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer{};
	try {
		VkResult result = vkAllocateCommandBuffers(bufferManager_logicalDevice, &allocInfo, &commandBuffer);
		if (result != VK_SUCCESS) {
			std::cerr << "[BufferManager] vkAllocateCommandBuffers failed: " << result << std::endl;
			throw std::runtime_error("Failed to allocate command buffer");
		}
		std::cout << "[beginOneTimeCommands] Allocated commandBuffer handle: " << commandBuffer << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "Exception during vkAllocateCommandBuffers: " << e.what() << std::endl;
	}

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
	if (result != VK_SUCCESS) {
		std::cerr << "[BufferManager] vkBeginCommandBuffer failed: " << result << std::endl;
		throw std::runtime_error("Failed to begin command buffer");
	} else {
		std::cout << "[BufferManager] vkBeginCommandBuffer succeeded." << std::endl;
	}

	return commandBuffer;
}


void BufferManager::endOneTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool commandPool) {
	VkResult result = vkEndCommandBuffer(commandBuffer);
	if (result != VK_SUCCESS) {
		std::cerr << "[BufferManager] vkEndCommandBuffer failed: " << result << std::endl;
		throw std::runtime_error("Failed to end command buffer");
	}
	else {
		std::cout << "[BufferManager] vkEndCommandBuffer succeeded." << std::endl;
	}

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	result = vkQueueSubmit(bufferManager_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	if (result != VK_SUCCESS) {
		std::cerr << "[BufferManager] vkQueueSubmit failed: " << result << std::endl;
		throw std::runtime_error("Failed to submit command buffer");
	}
	else {
		std::cout << "[BufferManager] vkQueueSubmit succeeded." << std::endl;
	}

	result = vkQueueWaitIdle(bufferManager_graphicsQueue);
	if (result != VK_SUCCESS) {
		std::cerr << "[BufferManager] vkQueueWaitIdle failed: " << result << std::endl;
		throw std::runtime_error("Failed to wait for queue idle");
	}
	else {
		std::cout << "[BufferManager] vkQueueWaitIdle succeeded." << std::endl;
	}

	vkFreeCommandBuffers(bufferManager_logicalDevice, commandPool, 1, &commandBuffer);
	std::cout << "[BufferManager] Command buffer freed." << std::endl;
}


void BufferManager::copyBuffer(
	std::shared_ptr<Buffer> srcBuffer, 
	std::shared_ptr<Buffer> dstBuffer, 
	VkDeviceSize size,
	VkCommandPool commandPool
) {
	std::cout << "Copy staging data into v/i buffer" << std::endl;
	VkCommandBuffer commandBuffer = beginOneTimeCommands(commandPool);

	//Copy buffer to dstBuffer
	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer->getHandle(), dstBuffer->getHandle(), 1, &copyRegion);

	endOneTimeCommands(commandBuffer, commandPool);
};

// == Deletion function == 
void BufferManager::removeBufferByName(const std::string name) {
	auto it = buffers.find(name);
	if (it != buffers.end()) {
		std::cout << "Removing buffer : [" << name << "]" << std::endl;
		buffers.erase(it);
	} else {
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

// == Cleanup functions == 
void BufferManager::cleanup() {
	std::cout << "    Destroying `BufferManager` " << std::endl;
	//Destroy all managed buffers - do the same for image manager and add .reset call to renderer cleanup
	for (auto& pair : buffers) {
		std::cout << "Cleaning buffer with key: " << pair.first << std::endl;
		pair.second->cleanup();
	}

	buffers.clear();
}