#include "../include/Buffer.h"

//THIS NEEDS TO BE MADE INTO SOME SORT OF HELPER FUNCTION
// MAYBE MOVE TO BUFFERUTILS
uint32_t Buffer::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties)) {
			return i;
		}
	};

	throw std::runtime_error("Failed to find suitable buffer memory type");
};

void Buffer::createBuffer(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
	VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
	VkDeviceSize size
) {
	allocateAndBindBuffer(logicalDevice, physicalDevice, size, usage, properties);
	// MAKE SURE THIS WORKS!!!
	mapData(logicalDevice, usage, properties);
};

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
};

template std::vector<Vertex> Buffer::getData<Vertex>() const;
template std::vector<uint32_t> Buffer::getData<uint32_t>() const;