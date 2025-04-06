//GLOBAL CONFIG FILE
//FOR GLOBAL VARIABLES AND LIBRARIES

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <optional>

#ifdef NDEBUG
	constexpr bool enableValidationLayers = false;
#else 
	constexpr  bool enableValidationLayers = true;
#endif