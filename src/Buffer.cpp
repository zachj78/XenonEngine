#include "../include/Buffer.h"
#include "../include/Image.h"
#include "../include/MemoryUtils.h"

void Buffer::createBuffer(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
	VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
	VkDeviceSize size
) {
	allocateAndBindBuffer(logicalDevice, physicalDevice, size, usage, properties);
	mapData(logicalDevice, usage, properties);
};

void Buffer::allocateAndBindBuffer(
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
		buf_errors |= BUF_ERROR_CREATION;
	};

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buf_handle, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &buf_memory) != VK_SUCCESS) {
		buf_errors |= BUF_ERROR_ALLOCATION;
	}

	if (vkBindBufferMemory(device, buf_handle, buf_memory, 0) != VK_SUCCESS) {
		buf_errors |= BUF_ERROR_BIND;
	};

	std::cout << "Allocated and binded buffer successfully" << std::endl;
}

void Buffer::mapData(VkDevice logicalDevice, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
	if (buf_type == BufferType::UNIFORM) return;
	if (buf_type == BufferType::GENERIC) return; 

	if (buf_type == BufferType::INDEX || buf_type == BufferType::VERTEX) {
		std::cout << "Successfully mapped data to buffer";
		return;
	};

	void* data;
	vkMapMemory(logicalDevice, buf_memory, 0, buf_size, 0, &data);
		
	//Map data to buffer depending on type: 
	if (buf_type == BufferType::VERTEX_STAGING) {
		memcpy(data, buf_vertices.value().data(), (size_t)buf_size);
	} else if (buf_type == BufferType::INDEX_STAGING) {
		memcpy(data, buf_indices.value().data(), (size_t)buf_size);
	}
		
	vkUnmapMemory(logicalDevice, buf_memory);
};


template<>
std::vector<Vertex> Buffer::getData<Vertex>() const {
	// for now we are only using staging buffers for vertex data, 
	// so we can safely assume any staging buffer holds some vertex data
	if (buf_type == BufferType::VERTEX_STAGING || buf_type == BufferType::VERTEX) {
		if (buf_vertices.has_value()) {
			return buf_vertices.value();
		}
	}

	throw std::runtime_error("No Index Data in " + buf_name);
}

template<>
std::vector<uint32_t> Buffer::getData<uint32_t>() const {
	if (buf_type == BufferType::INDEX_STAGING || buf_type == BufferType::INDEX) {
		if (buf_indices.has_value()) {
			return buf_indices.value();
		}
	}

	throw std::runtime_error("No Vertex Data in " + buf_name);
}

void Buffer::printErrors() const {
	if (buf_errors == BUF_ERROR_NONE) {
		std::cout << "[" << buf_name << "] No buffer errors detected." << std::endl;
		return;
	}

	std::cout << "[" << buf_name << "] Buffer Errors: " << std::endl;

	if (buf_errors & BUF_ERROR_CREATION) {
		std::cout << "  - Failed to create VkBuffer." << std::endl;
	}
	if (buf_errors & BUF_ERROR_ALLOCATION) {
		std::cout << "  - Failed to allocate buffer memory." << std::endl;
	}
	if (buf_errors & BUF_ERROR_COPY) {
		std::cout << "  - Failed to copy buffer data." << std::endl;
	}
	if (buf_errors & BUF_ERROR_BIND) {
		std::cout << "  - Failed to bind buffer memory." << std::endl;
	}

	if (buf_errors & BUF_ERROR_TYPE) {
		std::cout << "  - Buffer Type and data does not match" << std::endl;
	}
};

template std::vector<Vertex> Buffer::getData<Vertex>() const;
template std::vector<uint32_t> Buffer::getData<uint32_t>() const;