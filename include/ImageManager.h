#pragma once 
#ifndef IMAGE_MANAGER_H
#define IMAGE_MANAGER_H

#include "config.h"
#include "image.h"

//Forward declarations
class BufferManager; 
class Swapchain;


class ImageManager {
	public: 
		ImageManager(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, std::shared_ptr<Swapchain> swapchain, std::shared_ptr<BufferManager>);

		void createTextureImage(std::string name, std::string texturePath);
		void createDepthImage();

		// == Deletion function == 
		void removeImage(std::string name);

		// == Retreival functions == 
		std::shared_ptr<Image> getImage(std::string name);
		VkSampler getSampler(std::string name);
		VkImage getImageHandle(std::string name);
		ImageDetails getImageDetails(std::string name);
		
		VkImageView getDepthImageView() { return depthImages[0]->getImageDetails().imageView; }

		void cleanup();
		void cleanupDepthResources();

	private: 
		std::unordered_map<std::string, std::shared_ptr<Image>> images; 
		std::vector<std::shared_ptr<Image>> depthImages;

		VkDevice imageManager_logicalDevice;
		VkPhysicalDevice imageManager_physicalDevice;
		std::shared_ptr<Swapchain> imageManagerSwapchain;
		std::shared_ptr<BufferManager> imageManager_bufferManager;
};

#endif