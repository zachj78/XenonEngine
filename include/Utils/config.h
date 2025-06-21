//GLOBAL CONFIG FILE
//FOR GLOBAL VARIABLES(try to minimize unless absolute necessary**) AND PREPROCESSOR MACROS

#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define IMGUI_HAS_DOCK
#define IMGUI_ENABLE_DOCKING

//PLATFORM SPECIFIC MACROS -> VULKAN AND GLFW
#if defined(_WIN32)
	#pragma message("Detected Windows")
	#define GLFW_EXPLORE_NATIVE_WIN32
	#define VK_USE_PLATFORM_WIN32_KHR

#elif defined(__APPLE__) && defined(__MACH__)
	#pragma message("Detected MacOS")
	#define VK_USE_PLATFORM_MACOS_MVK
	#define GLFW_EXPOSE_NATIVE_COCOA

//THIS NEEDS TO BE UPDATED TO CONDITIONALLY SUPPORT XCB AS WELL
#elif defined(__linux__)
	#pragma message("Detected Linux")
	#define VK_USE_PLATFORM_WAYLAND_KHR
	#define GLFW_EXPOSE_NATIVE_WAYLAND

#else
	#error "Unsupported Platform"

#endif


#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "../src/External/imgui/imgui.h"
#include "../src/External/imgui/imgui_impl_vulkan.h"
#include "../src/External/imgui/imgui_impl_glfw.h"

#include <thread>
#include <queue>
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
#include <functional>
#include <unordered_map>
#include <utility>
#include <chrono>
#include <fstream>
#include <future>

//Platform Specific Includes
#if defined(_WIN32)
	#include <direct.h>
#else
	#include <unistd.h>
#endif

#ifdef NDEBUG
	constexpr bool enableValidationLayers = false;
#else 
	constexpr  bool enableValidationLayers = true;
#endif

constexpr int MAX_FRAMES_IN_FLIGHT = 2;