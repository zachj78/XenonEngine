#pragma once 
#ifndef IMAGE_MANAGER_H
#define IMAGE_MANAGER_H

#include "Utils/config.h"
#include "Managers/Image.h"

//Forward declarations
class BufferManager; 
class RenderTargeter;


class ImageManager {
	public: 
		ImageManager(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, std::shared_ptr<BufferManager>);

		//From relative file path
		void createTextureImage(std::string name, std::string texturePath, VkCommandPool commandPool);

		// From raw image data
		void createTextureImage(
			std::string name,
			std::vector<unsigned char>& pixels,
			int texWidth,
			int texHeight,
			VkCommandPool commandPool
		);

		// == Deletion function == 
		void removeImage(std::string name);

		// == Retrieval functions ==
		std::shared_ptr<Image> getImage(std::string name);
		VkSampler getSampler(std::string name);
		VkImage getImageHandle(std::string name);
		ImageDetails getImageDetails(std::string name);

		void cleanup();

	private: 
		std::unordered_map<std::string, std::shared_ptr<Image>> images; 

		VkDevice imageManager_logicalDevice;
		VkPhysicalDevice imageManager_physicalDevice;
		std::shared_ptr<RenderTargeter> imageManagerSwapchain;
		std::shared_ptr<BufferManager> imageManager_bufferManager;
};

#endif