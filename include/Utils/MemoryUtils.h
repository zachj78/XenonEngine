#pragma once
#ifndef MEMORY_UTILS_H
#define MEMORY_UTILS_H

#include "Utils/config.h"

//Finds memory type of the passed in physical device
uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
uint32_t verboseFindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

//Finds the supported image format -> ranks from least to most desirable
// Eventually this should be fixed to rank devies
VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& potentialFormats, VkImageTiling tiling, VkFormatFeatureFlags features);

VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);

#endif