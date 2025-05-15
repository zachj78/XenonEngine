#include "../include/ImageManager.h"
#include "../include/BufferManager.h"
#include "../include/Swapchain.h"

ImageManager::ImageManager(
	VkDevice logicalDevice, 
	VkPhysicalDevice physicalDevice, 
	std::shared_ptr<Swapchain> swapchain,
	std::shared_ptr<BufferManager> bufferManager
) : imageManager_logicalDevice(logicalDevice),imageManager_physicalDevice(physicalDevice), imageManagerSwapchain(swapchain), imageManager_bufferManager(bufferManager) 
{
	std::cout << "Successfully constructed ImageManager" << std::endl;
}

void ImageManager::createTextureImage(std::string name) {
	std::shared_ptr<Image> image = std::make_shared<Image>(imageManager_logicalDevice, imageManager_physicalDevice, imageManagerSwapchain, imageManager_bufferManager);
	image->createTextureImage();

	images[name] = std::move(image);
}

void ImageManager::createDepthImage() {
	std::shared_ptr<Image> depthImage = std::make_shared<Image>(imageManager_logicalDevice, imageManager_physicalDevice, imageManagerSwapchain, imageManager_bufferManager);

	depthImage->createDepthImage();
	depthImages.push_back(depthImage);
}

void ImageManager::removeImage(std::string name) {
	auto it = images.find(name);
	if (it != images.end()) {
		std::cout << "Removing image : [" << name << "]" << std::endl;
		images.erase(it);
	}
	else {
		std::cout << "Image [" << name << "] not found for removal" << std::endl;
	};
};

//Getter functions -> these get Image attributes via an image name
VkSampler ImageManager::getSampler(std::string name) {
	auto it = images.find(name);
	if (it != images.end()) {
		std::shared_ptr<Image> image = it->second;
		return image->getSampler();
	}
	else {
		std::cout << "Image [" << name << "Sampler not found" << std::endl;
	}
}

VkImage ImageManager::getImage(std::string name) {
	auto it = images.find(name);
	if (it != images.end()) {
		return it->second->getImage();
	}
	else {
		std::cout << "Image [" << name << "] not found for retreival" << std::endl;
	};
};

ImageDetails ImageManager::getImageDetails(std::string name) {
	auto it = images.find(name);
	if (it != images.end()) {
		std::shared_ptr<Image> image = it->second;
		return image->getImageDetails();
	}
	else {
		std::cout << "Image Details not found for [" << name << "]" << std::endl;
	}
};