#include "../include/VulkanDevices.h"

// ==Main functions==
void Devices::pickPhysicalDevice() {
	VkInstance instance = dev_instance->getInstance();
	VkSurfaceKHR surface = dev_instance->getSurface();

	// get count of total physical devices
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("Failed to find any GPUs with Vulkan support");
	};

	//Allocate an array to hold all queried devices
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	//Ensure all devices are suitable
	for (const auto& device : devices) {
		if (isDeviceSuitable(device)) {
			physicalDevice = device;
			break;
		}
	}; 

	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("Failed to find a suitable GPU");
	} else {
		logPhysicalDevice(); 
	};
};

void Devices::createLogicalDevice() {
	const std::vector<const char*> validationLayers = dev_instance->validationLayers;
	VkSurfaceKHR surface = dev_instance->getSurface();

	//First we create struct VkDeviceQueueCreateInfo
	// -> this struct specifies # of queues we want for a single queue family
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

	//Create createInfo structs for both our graphics and present queue
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	float queuePriority = 1.0f;

	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily; 
		queueCreateInfo.queueCount = 1; 
		queueCreateInfo.pQueuePriorities = &queuePriority; 
		
		//Add create info struct to queueCreateInfos vector
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	// we are only using graphics queue family currently
	queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value(); 
	queueCreateInfo.queueCount = 1; 

	//This effects the scheduling of command buffer execution if you have multiple queueFamilies
	queueCreateInfo.pQueuePriorities = &queuePriority;
	
	//Specifies device features we will be using, 
	// -> not used now 
	VkPhysicalDeviceFeatures deviceFeatures{};

	//Main DeviceCreateInfo struct 
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()); 
	createInfo.pEnabledFeatures = &deviceFeatures; 

	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	};

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create a logical device");
	} else {
		std::cout << "Created logical device" << std::endl;
	};

	//store our graphics queue family in a VkQueue handle
	vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
	//as well as our present queue
	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
};

//Destructor, destroys logical device
void Devices::cleanup() {
	//Destroy logical device
	vkDestroyDevice(device, nullptr);
};

// ==HELPER FUNCTIONS==

//Checks device to ensure all specified extensions(VulkanDevices::deviceExtensions) are supported
const bool Devices::checkDeviceExtensionSupport(VkPhysicalDevice potentialDevice) {
	//Get count of extensions supported by physical device
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(potentialDevice, nullptr, &extensionCount, nullptr);

	//Create an array of supported device extensions
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(potentialDevice, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	};

	return requiredExtensions.empty();

}

//This function checks if a given physical device is suitable
const bool Devices::isDeviceSuitable(VkPhysicalDevice potentialDevice) {
	VkSurfaceKHR surface = dev_instance->getSurface();

	//Check that device extensions are supported
	bool extensionsSupported = checkDeviceExtensionSupport(potentialDevice);

	bool swapchainAdaquate = false; 
	if (extensionsSupported) {
		SwapchainSupportDetails swapchainSupport = querySwapchainSupport(potentialDevice, surface);
		swapchainAdaquate = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
	}

	//Query for basic device properties(name, type, supported vulkan version)
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(potentialDevice, &deviceProperties);

	//Query for basic device features(texture compression, multiviewport rendering)
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(potentialDevice, &deviceFeatures);

	//Query our queue familes, assign to QueueFamilyIndices
	QueueFamilyIndices indices = findQueueFamilies(potentialDevice, surface);

	// returns true for discrete gpus or integrated gpus with geometry shader capabilities
	// also ensures we have found a indice for a graphics queue family
	return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
		&& deviceFeatures.geometryShader && indices.isComplete() && extensionsSupported && swapchainAdaquate;
};

// -- Logger functions -- 
void Devices::logPhysicalDevice() const {
	//log device properties and name
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;

	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

	printf("Found Phyiscal Device: \n Name: ", deviceProperties.deviceName,
		"Type: ", deviceProperties.deviceType);

	printf("  API Version: %d.%d.%d\n",
		VK_VERSION_MAJOR(deviceProperties.apiVersion),
		VK_VERSION_MINOR(deviceProperties.apiVersion),
		VK_VERSION_PATCH(deviceProperties.apiVersion));

	printf("  Supports Geometry Shader: %s\n",
		deviceFeatures.geometryShader ? "Yes" : "No");
	printf("  Supports Tessellation Shader: %s\n",
		deviceFeatures.tessellationShader ? "Yes" : "No");
	printf("  Max Image Dimension 2D: %u\n",
		deviceProperties.limits.maxImageDimension2D);
};