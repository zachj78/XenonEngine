#include "../include/MemoryUtils.h"

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties)) {
			return i;
		}
	};

	throw std::runtime_error("Failed to find suitable buffer memory type");
};

uint32_t verboseFindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    std::cout << "Searching for suitable memory type..." << std::endl;

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        const VkMemoryType& memoryType = memProperties.memoryTypes[i];

        // Print memory type info for debugging
        std::cout << "Checking memory type " << i << ": "
            << "Heap Index: " << memoryType.heapIndex
            << ", Property Flags: " << memoryType.propertyFlags << std::endl;

        // Check if the memory type is compatible with the typeFilter and properties
        if ((typeFilter & (1 << i)) && (memoryType.propertyFlags & properties) == properties) {
            std::cout << "Found suitable memory type: " << i << std::endl;
            return i;
        }
        else {
            std::cout << "Memory type " << i << " is not suitable." << std::endl;
        }
    }

    // If no memory type was found, print detailed error and throw exception
    std::cerr << "Failed to find suitable memory type. Possible options:" << std::endl;
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        const VkMemoryType& memoryType = memProperties.memoryTypes[i];
        std::cerr << "Memory type " << i << ": "
            << "Heap Index: " << memoryType.heapIndex
            << ", Property Flags: " << memoryType.propertyFlags << std::endl;
    }

    throw std::runtime_error("Failed to find suitable buffer memory type");
};


VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& potentialFormats, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : potentialFormats) {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);

        if (tiling == VK_IMAGE_TILING_LINEAR && (formatProperties.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (formatProperties.optimalTilingFeatures & features) == features) {
            return format;
        };

    };
   
    throw std::runtime_error("Failed to find supported image format");
};

const char* vkFormatToString(VkFormat format) {
    switch (format) {
    case VK_FORMAT_D32_SFLOAT: return "VK_FORMAT_D32_SFLOAT";
    case VK_FORMAT_D32_SFLOAT_S8_UINT: return "VK_FORMAT_D32_SFLOAT_S8_UINT";
    case VK_FORMAT_D24_UNORM_S8_UINT: return "VK_FORMAT_D24_UNORM_S8_UINT";
    case VK_FORMAT_R8G8B8A8_SRGB: return "VK_FORMAT_R8G8B8A8_SRGB";
    case VK_FORMAT_R8G8B8_SRGB: return "VK_FORMAT_R8G8B8_SRGB";
        // Add others as needed
    default: return "UNKNOWN_FORMAT";
    }
}

VkFormat findDepthFormat(VkPhysicalDevice physicalDevice) {
    VkFormat supportedFormat = findSupportedFormat(
        physicalDevice,
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );

    std::cout << "Using format: " << vkFormatToString(supportedFormat) << std::endl;
    return supportedFormat;
};
