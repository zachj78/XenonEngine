#include "helperFuncs.h"

std::vector<const char*> getRequiredExtensions() {
	//Stores total number of extension strings
	uint32_t glfwExtensionCount = 0;
	//Pointer to an array of c-strings(each string is const char*)
	//when using indexing it points to the arrays first c-string(extension)
	const char** glfwExtensions;
	//Returns a pointer to an array of c-strings
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	};

	return extensions;
};