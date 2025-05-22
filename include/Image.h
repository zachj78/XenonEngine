#pragma once 
#ifndef IMAGE_H
#define IMAGE_H

#include "config.h"

//Forward declarations
class BufferManager;
class Swapchain;

//Holds image metadata
struct ImageDetails {
	VkImageView imageView = VK_NULL_HANDLE;
	VkFormat imageFormat = VK_FORMAT_UNDEFINED;
	VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	uint32_t imageWidth = 0;
	uint32_t imageHeight = 0;
	VkImageAspectFlags imageAspectFlags = 0;
};

enum ImageErrors : unsigned short {
	IMG_ERROR_NONE = 0,
	IMG_ERROR_CREATION = 0b0000000000000001,
	IMG_ERROR_ALLOCATION = 0b0000000000000010,
	IMG_ERROR_COPY = 0b0000000000000100,
	IMG_ERROR_BIND = 0b0000000000001000,
	IMG_ERROR_TYPE = 0b0000000000010000, // this can be used later when certain formats and layouts are expected for different types of images
	IMG_ERROR_LOAD = 0b0000000000100000,
	IMG_ERROR_VIEW_CREATION = 0b0000000001000000, 
	IMG_ERROR_SAMPLER_CREATION = 0b0000000010000000
};

class Image {
public:
	Image(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, std::shared_ptr<Swapchain> swapchain, std::shared_ptr<BufferManager> bufferManager);

	void cleanup() {
		std::cout << "Cleaned up image: " << image << " successfully" << std::endl;

		if (imageSampler != VK_NULL_HANDLE) {
			vkDestroySampler(imageLogicalDevice, imageSampler, nullptr);
			imageSampler = VK_NULL_HANDLE;
		}

		if (imageDetails.imageView != VK_NULL_HANDLE) {
			vkDestroyImageView(imageLogicalDevice, imageDetails.imageView, nullptr);
			imageDetails.imageView = VK_NULL_HANDLE;
		}

		if (image != VK_NULL_HANDLE) {
			vkDestroyImage(imageLogicalDevice, image, nullptr);
			image = VK_NULL_HANDLE;
		}

		if (imageMemory != VK_NULL_HANDLE) {
			vkFreeMemory(imageLogicalDevice, imageMemory, nullptr);
			imageMemory = VK_NULL_HANDLE;
		}
	}


	//This function will fully create a texture image from the main script
	void createTextureImage(std::string texturePath);

	//This functions will full create a depth image from the main script
	void createDepthImage();

	void createImage(uint32_t width, uint32_t height,
		VkImageTiling imageTiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties
	);
	void transitionImageLayout(VkImageLayout newLayout);
	void copyBufferToImage(VkBuffer buffer);
	void createImageView();

	//Specific texture image util functions

	void createTextureSampler();

	//Specific depth image util functions

	void printErrors() const;

	//Getter functions (these are called from imageManager- which should be exposed to the main script)
	VkSampler getSampler() { return imageSampler; };
	VkImage getImage() { return image; };
	ImageDetails getImageDetails() { return imageDetails; };

private:
	//Injected Vulkan Core components
	VkDevice imageLogicalDevice; 
	VkPhysicalDevice imagePhysicalDevice; 
	std::shared_ptr<Swapchain> imageSwapchain; 
	//Injected custom managers
	std::shared_ptr<BufferManager> imageBufferManager;

	//Main variables
	VkImage image; 
	VkDeviceMemory imageMemory; 
	ImageDetails imageDetails; 

	VkSampler imageSampler = VK_NULL_HANDLE;
	unsigned short imageErrors = IMG_ERROR_NONE; 
};

#endif