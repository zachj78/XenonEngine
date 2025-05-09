#pragma once
#ifndef BUFFER_H
#define BUFFER_H

#include "config.h"
#include "MeshManager.h"

enum BufferErrors : unsigned short {
	BUF_ERROR_NONE = 0,
	BUF_ERROR_CREATION = 0b0000000000000001,
	BUF_ERROR_ALLOCATION = 0b0000000000000010,
	BUF_ERROR_COPY = 0b0000000000000100, 
	BUF_ERROR_BIND = 0b0000000000001000,
};

enum class BufferType {
	VERTEX, 
	INDEX,
	VERTEX_STAGING,
	INDEX_STAGING,
	UNIFORM,
	GENERIC,
	GENERIC_STAGING
};

/**
	* @class Buffer
	* @brief Represents a vertex, index, staging or depth buffer, and manages it's creation, memroy allocation and operations.
	*
	* The `Buffer` class is responsible for creating and managing a single Vulkan Buffer(`VkBuffer`) object and handles it's lifecycle.
	* including creation, memory allocation and data management. 
	* 
	* This class provides per-buffer operations, such as the ability to copy a member variable VkBuffer to a destination buffer.
	*
	* This class is primarily used by the `BufferManager` class, which stores all buffers in an unordered map
	* using a name- set from the meshManager for easy retreival.
	* 
	* @note The `Buffer` class assumes that the Vulkan device, physical device and command pool 
	* are already initialized. The BufferManager is expected to manage the buffers, and this class should not be used in isolation.
	* 
	* @example: 
	* 
*/
class Buffer{
public: 

	//Helper
	uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

	Buffer(BufferType type, 
		const std::string& name,
		VkDeviceSize size,
		VkDevice logicalDevice,
		VkPhysicalDevice physicalDevice, 
		std::optional<std::vector<Vertex>> vertices,
		std::optional<std::vector<uint32_t>> indices)
	{
		buf_type = type; 
		buf_name = name; 
		buf_size = size;
		buf_logicalDevice = logicalDevice;
		buf_physicalDevice = physicalDevice;

		if ((type == BufferType::VERTEX || type == BufferType::VERTEX_STAGING) && vertices.has_value()) {
			buf_vertices = vertices;
		} else {
			buf_vertices = std::nullopt;
		};

		if ((type == BufferType::INDEX || type == BufferType::INDEX_STAGING) && indices.has_value()) {
			buf_indices = indices;
		} else {
			buf_indices = std::nullopt; 
		}
	};

	void allocateAndBindBuffer(
		VkDevice device,
		VkPhysicalDevice physicalDevice,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties
	) {
		std::cout << "Allocating buffer of size : [" << size << "]" << std::endl;

		// Create buffer
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buf_handle) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buf_handle, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &buf_memory) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate buffer memory!");
		}

		if (vkBindBufferMemory(device, buf_handle, buf_memory, 0) != VK_SUCCESS) {
			throw std::runtime_error("Failed to bind buffer memory!");
		};

		std::cout << "Allocated and binded buffer successfully" << std::endl;
	}
	
	void Buffer::createBuffer(
		VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
		VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
		VkDeviceSize size
	);

	void mapData(VkDevice logicalDevice, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

	//Error handling functions
	bool hasErrors() const { return buf_errors != BUF_ERROR_NONE; };
	void setError(BufferErrors error) { buf_errors |= error; };
	void printErrors() const;

	//Getter functions 
	VkBuffer getHandle() const { return buf_handle; };
	VkDeviceMemory getMemory() const { return buf_memory; };

	//Create functions that automatically map data for index or vertex buffers,

	//This function gets the data within a buffer, either vertices or indices(for now)
	template<typename T>
	std::vector<T> getData() const;

private: 
	//Injected vulkan components - for internal method use
	VkDevice buf_logicalDevice;
	VkPhysicalDevice buf_physicalDevice;


	//Buffer info
	BufferType buf_type = BufferType::GENERIC;
	std::string buf_name;

	VkDeviceSize buf_size; 
	VkBuffer buf_handle = VK_NULL_HANDLE;
	VkDeviceMemory buf_memory = VK_NULL_HANDLE;
	unsigned short buf_errors = BUF_ERROR_NONE;

	//Optional data
	std::optional<std::vector<Vertex>> buf_vertices;
	std::optional<std::vector<uint32_t>> buf_indices;
};

#endif