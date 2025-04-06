#ifndef HELPER_FUNCS_H
#define HELPER_FUNCS_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

//Config file
#include "config.h"

#include <vector>

/*
	This function returns a list of required Vulkan extensions(char* strings) for GLFW
	based on whether validation layers are enabled or not
*/
std::vector<const char*> getRequiredExtensions();

/* 
	This is a basic loader function
	-> returns an VkInstance extension function's address with "vkGetInstanceProcAddr"
*/
template<typename T>
T LoadInstanceFunction(VkInstance instance, const char* name) {
	return reinterpret_cast<T>(vkGetInstanceProcAddr(instance, name));
};

#endif