#ifndef DEBUG_FUNCS_H
#define DEBUG_FUNCS_H

#include<iostream>
#include <vector>
#include <vulkan/vulkan.h>

void DBG_checkAllLayers(const std::vector<VkLayerProperties>& availableLayers);

#endif