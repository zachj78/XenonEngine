//GLOBAL CONFIG FILE
//FOR GLOBAL VARIABLES AND LIBRARIES

#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPLORE_NATIVE_WIN32
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
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
#include <array>
#include <string>
#include <direct.h>
#include <functional>
#include <unordered_map>
#include <utility>
#include <chrono>


#ifdef NDEBUG
	constexpr bool enableValidationLayers = false;
#else 
	constexpr  bool enableValidationLayers = true;

constexpr int MAX_FRAMES_IN_FLIGHT = 2;


#endif