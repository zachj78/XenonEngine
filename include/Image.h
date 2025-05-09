//#pragma once 
//#ifndef IMAGE_H
//#define IMAGE_H
//#include <config.h>
//#include "BufferManager.h"
//
//#define STB_IMAGE_IMPLEMENTATION
//#include <stb_image.h>
//
//class Image {
//public:
//	Image(VkDevice logicalDevice, std::shared_ptr<BufferManager> bufferManager)
//		: imageLogicalDevice(logicalDevice), imageBufferManager(bufferManager) {
//	};
//
//	//void createTextureImage() {
//	//	int texWidth, texHeight, texChannels;
//
//	//	stbi_uc* pixels = stbi_load("textures/wall.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
//	//	VkDeviceSize imageSize = texWidth * texHeight * 4;
//
//	//	if (!pixels) {
//	//		throw std::runtime_error("Failed to load texture image!");
//	//	};
//
//	//	imageBufferManager->createRawBuffer(
//	//		BufferType::GENERIC_STAGING,
//	//		"texImage_staging",
//	//		imageSize,
//	//		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//	//		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
//	//	);
//
//	//	Buffer* stagingBuf = imageBufferManager->getBuffer("texImage_staging");
//
//	//	void* data;
//	//	vkMapMemory(imageLogicalDevice, stagingBuf->buf_memory, 0, imageSize, 0, &data);
//	//	memcpy(data, pixels, static_cast<size_t>(imageSize));
//	//	vkUnmapMemory(imageLogicalDevice, stagingBuf->buf_memory);
//
//	//	VkImageCreateInfo imageInfo{};
//	//	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
//	//	imageInfo.imageType = VK_IMAGE_TYPE_2D;
//	//	imageInfo.extent.width = static_cast<uint32_t>(texWidth);
//	//	imageInfo.extent.height = static_cast<uint32_t>(texHeight);
//	//	imageInfo.extent.depth = 1;
//	//	imageInfo.mipLevels = 1;
//	//	imageInfo.arrayLayers = 1;
//
//	//	imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
//	//	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
//
//	//	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//	//	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
//	//	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//	//	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
//	//	imageInfo.flags = 0;
//
//	//	VkResult createTexImageResult = vkCreateImage(imageLogicalDevice, &imageInfo, nullptr, &textureImage);
//
//	//	if (createTexImageResult != VK_SUCCESS) {
//	//		throw std::runtime_error("Failed to create texture image");
//	//	} else {
//	//		std::cout << "Texture image created -> size :[" << imageSize << "], result: [" << createTexImageResult << "]" << std::endl;
//	//	};
//
//	//	//Allocate memory for image
//	//	VkMemoryRequirements memRequirements; 
//	//	vkGetImageMemoryRequirements(imageLogicalDevice, textureImage, &memRequirements);
//
//	//	VkMemoryAllocateInfo allocInfo{};
//	//	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//	//	allocInfo.allocationSize = memRequirements.size;
//	//	allocInfo.memoryTypeIndex = findMemoryType()
//	//}
//private:
//	//Injected Vulkan Core components
//	VkDevice imageLogicalDevice; 
//	//Injected custom managers
//	std::shared_ptr<BufferManager> imageBufferManager;
//
//	//Main variables
//	VkImage textureImage; 
//	VkDeviceMemory textureImageMemory; 
//};
//
//#endif