//GLOBAL CONFIG FILE
//FOR GLOBAL VARIABLES AND LIBRARIES

#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPLORE_NATIVE_WIN32
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>

#ifdef NDEBUG
	constexpr bool enableValidationLayers = false;
#else 
	constexpr  bool enableValidationLayers = true;
#endif