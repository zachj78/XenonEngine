#include "../include/Managers/ImageManager.h"
#include "../include/Managers/BufferManager.h"

#include "../include/External/stb_image.h"

ImageManager::ImageManager(
	VkDevice logicalDevice, 
	VkPhysicalDevice physicalDevice, 
	std::shared_ptr<BufferManager> bufferManager
) : imageManager_logicalDevice(logicalDevice),imageManager_physicalDevice(physicalDevice), imageManager_bufferManager(bufferManager) 
{
	std::cout << "Successfully constructed ImageManager" << std::endl;
	std::cout << "     with logical device: " << imageManager_logicalDevice << std::endl;
	std::cout << "     and bufferManager : " << bufferManager << std::endl;
}

void ImageManager::createTextureImage(std::string name, std::string texturePath, VkCommandPool commandPool) {
	std::cout << "ImageManager::createTextureImage entered" << std::endl;

	std::shared_ptr<Image> image = std::make_shared<Image>(imageManager_logicalDevice, imageManager_physicalDevice);
	
	//Create a staging buffer for the new image: 
	std::string texImageStagingName = "texImage_staging";

	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	std::cout << "imageManager::createTextureImage imageSize: " << imageSize << std::endl;

	imageManager_bufferManager->createBuffer(
		BufferType::GENERIC_STAGING,
		texImageStagingName,
		imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	std::shared_ptr<Buffer> stagingBuf = imageManager_bufferManager->getBuffer("texImage_staging");

	std::cout << "imageManager::createTextureImage stagingBuffer handle: " << stagingBuf->getHandle() << std::endl;

	image->createTextureImage(pixels, texWidth, texHeight, commandPool, stagingBuf, imageManager_bufferManager);
	
	std::cout << "[Created texture image] : " << name << std::endl;

	stbi_image_free(pixels);

	images[name] = std::move(image);
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

VkImage ImageManager::getImageHandle(std::string name) {
	auto it = images.find(name);
	if (it != images.end()) {
		return it->second->getImage();
	}
	else {
		std::cout << "Image [" << name << "] not found for retreival" << std::endl;
	};
};

std::shared_ptr<Image> ImageManager::getImage(std::string name) {
	auto it = images.find(name);
	if (it != images.end()) {
		return it->second;
	}
	else {
		std::cout << "Image [" << name << "] not found for retreival" << std::endl;
	};
}

void ImageManager::cleanup() {
	std::cout << "    Destroying `ImageManager` " << std::endl;
	for (auto& pair : images) {
		if (pair.second) {
			std::cout << "Calling cleanup on image handle: " << pair.first << std::endl;
			pair.second->cleanup();
		}
	}

	images.clear();
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