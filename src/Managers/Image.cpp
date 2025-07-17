#include "../include/Managers/Image.h"
#include "../include/Managers/BufferManager.h"
#include "../include/Core/Swapchain.h"
#include "../include/Utils/MemoryUtils.h"
#include "../include/Managers/Buffer.h"

// == HELPER FUNCTIONS ==

// Checks if a depth image format contains a stencil component
bool hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
};

Image::Image(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice
)
	: imageLogicalDevice(logicalDevice), imagePhysicalDevice(physicalDevice) {
		std::cout << "Constructed image" << std::endl;
};

void Image::createTextureImage(
	stbi_uc* pixels,
	int texWidth, 
	int texHeight,
	VkCommandPool commandPool, 
	std::shared_ptr<Buffer> stagingBuf, 
	std::shared_ptr<BufferManager> bufferManager
) {
	//DEBUG
	std::cout << "Image::createTextureImage called" << std::endl;
	std::cout << "Image::commandPool : [" << commandPool << "]" << std::endl;

	imageDetails.currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	//Create an image view for the texture image
	imageDetails.imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
	imageDetails.imageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels) {
		imageErrors |= IMG_ERROR_LOAD;
		throw std::runtime_error("Failed to load texture image!");
	} else {
		std::cout << "image size: " << imageSize << std::endl;
	};

	void* data;
	vkMapMemory(imageLogicalDevice, stagingBuf->getMemory(), 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(imageLogicalDevice, stagingBuf->getMemory());

	createImage(texWidth, texHeight, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, commandPool, bufferManager);
	copyBufferToImage(stagingBuf->getHandle(), commandPool, bufferManager);
	transitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, commandPool, bufferManager);

	createImageView();
	createTextureSampler();
};

void Image::createDepthImage(VkExtent2D renderTargetExtent) {
	imageDetails.imageFormat = findDepthFormat(imagePhysicalDevice);
	imageDetails.imageAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;

	//Now we create image using the found depth format
	uint32_t width = renderTargetExtent.width;
	uint32_t height = renderTargetExtent.height;
	
	createImage(width, height,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	createImageView();
}

void Image::createImage(uint32_t width, uint32_t height,
	VkImageTiling imageTiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags properties
) {
	imageDetails.imageWidth = width;
	imageDetails.imageHeight = height;

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = static_cast<uint32_t>(width);
	imageInfo.extent.height = static_cast<uint32_t>(height);
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;

	imageInfo.format = imageDetails.imageFormat;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

	imageInfo.initialLayout = imageDetails.currentLayout;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	VkResult createImageResult = vkCreateImage(imageLogicalDevice, &imageInfo, nullptr, &image);

	if (createImageResult != VK_SUCCESS) {
		imageErrors |= IMG_ERROR_CREATION;
	} else {
		std::cout << "Image created -> size :[" << width * height * 4 << "], result: [" << createImageResult << "]" << std::endl;
	};

	//Allocate memory for image
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(imageLogicalDevice, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = verboseFindMemoryType(imagePhysicalDevice, memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(imageLogicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		imageErrors |= IMG_ERROR_ALLOCATION;
	} else {
		std::cout << "Allocated image memory successfully" << std::endl;
	};

	//Bind image to memory
	VkResult bindImageMemResult = vkBindImageMemory(imageLogicalDevice, image, imageMemory, 0);

	if (bindImageMemResult != VK_SUCCESS) {
		imageErrors |= IMG_ERROR_BIND;
	} else {
		std::cout << "Successfully bound image memory" << std::endl;
	}
};

void Image::transitionImageLayout(VkImageLayout newLayout, VkCommandPool commandPool, std::shared_ptr<BufferManager> bufferManager) {
	std::cout << "[Image::transitionImageLayout] Entering function\n";
	std::cout << "[Image::transitionImageLayout] VkImage handle: " << image << std::endl;
	std::cout << "[Image::transitionImageLayout] Current layout: " << imageDetails.currentLayout
		<< ", New layout: " << newLayout << std::endl;
	std::cout << "[Image::transitionImageLayout] commnadPool : [" << commandPool << "]" << std::endl;

	VkCommandBuffer commandBuffer = bufferManager->beginOneTimeCommands(commandPool);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = imageDetails.currentLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (imageDetails.currentLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
		newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		std::cout << "[Image::transitionImageLayout] Transition: UNDEFINED → TRANSFER_DST_OPTIMAL\n";

		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (imageDetails.currentLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {

		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		std::cout << "[Image::transitionImageLayout] Transition: TRANSFER_DST_OPTIMAL → SHADER_READ_ONLY_OPTIMAL\n";
	} else {
		imageErrors |= IMG_ERROR_TYPE;
		throw std::invalid_argument("Unsupported layout transition in `Image::transitionImageLayout()`");
	}

	vkCmdPipelineBarrier(commandBuffer,
		sourceStage, destinationStage,
		0, 0, nullptr, 0, nullptr, 1, &barrier
	);

	bufferManager->endOneTimeCommands(commandBuffer, commandPool);
	imageDetails.currentLayout = newLayout; // Update the layout state
	std::cout << "[Image::transitionImageLayout] Layout transition completed.\n";
}

void Image::copyBufferToImage(VkBuffer buffer, VkCommandPool commandPool, std::shared_ptr<BufferManager> bufferManager) {
	std::cout << "[Image::copyBufferToImage] Copying buffer to image\n";
	std::cout << "[Image::copyBufferToImage] VkBuffer: " << buffer << ", VkImage: " << image << std::endl;
	std::cout << "[Image::copyBufferToImage] Dimensions: " << imageDetails.imageWidth << "x" << imageDetails.imageHeight << std::endl;

	if (buffer == VK_NULL_HANDLE || image == VK_NULL_HANDLE) {
		imageErrors |= IMG_ERROR_COPY;
	}

	VkCommandBuffer commandBuffer = bufferManager->beginOneTimeCommands(commandPool);

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		imageDetails.imageWidth, imageDetails.imageHeight, 1
	};

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	bufferManager->endOneTimeCommands(commandBuffer, commandPool);
	std::cout << "[Image::copyBufferToImage] Copy completed.\n";
}

void Image::createImageView() {
	std::cout << "Image::createImageView() entered" << std::endl;

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = imageDetails.imageFormat;
	viewInfo.subresourceRange.aspectMask = imageDetails.imageAspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (hasStencilComponent(imageDetails.imageFormat)) {
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	} else if (imageDetails.imageAspectFlags & VK_IMAGE_ASPECT_DEPTH_BIT) {
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	} else {
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	VkResult imageViewResult = vkCreateImageView(imageLogicalDevice, &viewInfo, nullptr, &imageDetails.imageView);

	if (imageViewResult == VK_SUCCESS) {
		std::cout << "Successfully created image view" << std::endl;
	} else {
		imageErrors |= IMG_ERROR_VIEW_CREATION;
	};

	std::cout << "Created image view(s) : " << imageDetails.imageView << std::endl;
};

// Specific texture image functions 
void Image::createTextureSampler() {
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	//Specifies the behavior of the image when it goes beyond it's dimensions
	// in this case it will repeat the image
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	samplerInfo.anisotropyEnable = VK_FALSE; // set false just for now

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(imagePhysicalDevice, &properties);
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	VkResult createSamplerResult = vkCreateSampler(imageLogicalDevice, &samplerInfo, nullptr, &imageSampler);

	if (createSamplerResult == VK_SUCCESS) {
		std::cout << "Successfully created image sampler" << std::endl;
	} else {
		imageErrors |= IMG_ERROR_SAMPLER_CREATION;
	}
}

void Image::printErrors() const {
	if (imageErrors == IMG_ERROR_NONE) {
		//store a name dumbass
		std::cout << "[" << "[IMAGE NAME]" << "] No buffer errors detected." << std::endl;
		return;
	}

	std::cout << "[" << "[IMAGE NAME]" << "] Buffer Errors: " << std::endl;

	if (imageErrors & IMG_ERROR_CREATION) {
		std::cout << "  - Failed to create VkImage." << std::endl;
	}
	if (imageErrors & IMG_ERROR_ALLOCATION) {
		std::cout << "  - Failed to allocate image memory." << std::endl;
	}
	if (imageErrors & IMG_ERROR_COPY) {
		std::cout << "  - Failed to copy buffer to image \n     -> Invalid resouces(buffer and/or image) provided" << std::endl;
	}
	if (imageErrors & IMG_ERROR_BIND) {
		std::cout << "  - Failed to bind image memory." << std::endl;
	}

	if (imageErrors & IMG_ERROR_VIEW_CREATION) {
		std::cout << "  - Failed to create VkImageView" << std::endl;
	}

	if (imageErrors & IMG_ERROR_SAMPLER_CREATION) {
		std::cout << "  - Failed to create VkSampler" << std::endl;
	} 
};