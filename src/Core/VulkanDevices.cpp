#include "../include/Core/VulkanDevices.h"

void Capabilities::query(VkPhysicalDevice potentialDevice) {
	VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{};
	indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;

	VkPhysicalDeviceFeatures2 features2{};
	features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features2.pNext = &indexingFeatures;

	vkGetPhysicalDeviceFeatures2(potentialDevice, &features2);

	runtimeDescriptorArray = indexingFeatures.runtimeDescriptorArray;
	shaderSampledImageArrayNonUniformIndexing = indexingFeatures.shaderSampledImageArrayNonUniformIndexing;
	descriptorBindingPartiallyBound = indexingFeatures.descriptorBindingPartiallyBound;
	descriptorBindingVariableDescriptorCount = indexingFeatures.descriptorBindingVariableDescriptorCount;

	supportsBindless =
		runtimeDescriptorArray &&
		shaderSampledImageArrayNonUniformIndexing &&
		descriptorBindingPartiallyBound &&
		descriptorBindingVariableDescriptorCount;

	// Optionally query limits
	VkPhysicalDeviceDescriptorIndexingProperties indexingProps{};
	indexingProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES;

	VkPhysicalDeviceProperties2 props2{};
	props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	props2.pNext = &indexingProps;

	vkGetPhysicalDeviceProperties2(potentialDevice, &props2);
	maxUpdateAfterBindDescriptorsInAllPools = indexingProps.maxUpdateAfterBindDescriptorsInAllPools;
}

// ==Main functions==
void Devices::pickPhysicalDevice() {
	std::cout << "Picking physical device" << std::endl;

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

	int deviceScore = -1; 
	//Ensure all devices are suitable
	for (const auto& device : devices) {
		bool isScoreGreater = rateDeviceSuitability(device, deviceScore);

		if (isScoreGreater) {
			physicalDevice = device;
		};
	}; 

	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("Failed to find a suitable GPU");
	} else {
		logPhysicalDevice(); 
	};
};

void Devices::createLogicalDevice() {
	std::cout << "Creating logical device" << std::endl;

	const auto& validationLayers = dev_debugManager->getValidationLayers();
	VkSurfaceKHR surface = dev_instance->getSurface();

	//First we create struct VkDeviceQueueCreateInfo
	// -> this struct specifies # of queues we want for a single queue family
	queueFamilies = findQueueFamilies(physicalDevice, surface);

	//Create createInfo structs for both our graphics and present queue
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { queueFamilies.graphicsFamily.value(), queueFamilies.presentFamily.value() };

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
	queueCreateInfo.queueFamilyIndex = queueFamilies.graphicsFamily.value();
	queueCreateInfo.queueCount = 1; 

	//This effects the scheduling of command buffer execution if you have multiple queueFamilies
	queueCreateInfo.pQueuePriorities = &queuePriority;
	
	//Specifies device features we will be using, 
	// -> not used now 
	VkPhysicalDeviceFeatures deviceFeatures{};

	VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{};
	VkDeviceCreateInfo createInfo{};
	VkPhysicalDeviceFeatures2 deviceFeatures2{};

	if (deviceCaps.supportsBindless) {
		//VULKAN 1.2 FEATURES
		VkPhysicalDeviceVulkan12Features vulkan12Features{};
		vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		vulkan12Features.runtimeDescriptorArray = VK_TRUE;
		vulkan12Features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;

		deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		deviceFeatures2.features = deviceFeatures; 
		deviceFeatures2.pNext = &vulkan12Features; 

		createInfo.pNext = &deviceFeatures2;
		createInfo.pEnabledFeatures = nullptr;

	} else {
		createInfo.pNext = nullptr;
		createInfo.pEnabledFeatures = &deviceFeatures;
	}
	
	//Main DeviceCreateInfo struct 
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()); 
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

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
	vkGetDeviceQueue(device, queueFamilies.graphicsFamily.value(), 0, &graphicsQueue);
	//as well as our present queue
	vkGetDeviceQueue(device, queueFamilies.presentFamily.value(), 0, &presentQueue);
};

//Destructor, destroys logical device
void Devices::cleanup() {
	std::cout << "    Destroying `Devices` " << std::endl;
	vkDestroyDevice(device, nullptr);
};

// ==HELPER FUNCTIONS==
//Checks device to ensure all specified extensions(VulkanDevices::deviceExtensions) are supported
//THIS NEEDS TO BE CHANGED TO CONFIGURE EXTENSIONS -> SET A VARIABLE LIKE indexingAvailable IF ALL INDEXING EXTENSIONS ARE SUUPORTED AND CHANGE DEVICE EXTENSIONS
const bool Devices::checkDeviceExtensionSupport(VkPhysicalDevice potentialDevice) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(potentialDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(potentialDevice, nullptr, &extensionCount, availableExtensions.data());

	std::cout << "Available device extensions:" << std::endl;
	for (const auto& ext : availableExtensions) {
		std::cout << "  " << ext.extensionName << std::endl;
	}

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	if (!requiredExtensions.empty()) {
		std::cout << "Missing required device extensions:" << std::endl;
		for (const auto& missing : requiredExtensions) {
			std::cout << "  " << missing << std::endl;
		}
	}

	return requiredExtensions.empty();
}

//This function modifies extensions and features in order to make a device suitable
const bool Devices::rateDeviceSuitability(VkPhysicalDevice potentialDevice, int currentGreatestDeviceScore) {
	int score = 0;
	Capabilities caps;
	caps.query(potentialDevice);

	std::vector<const char*> requiredExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	deviceExtensions = requiredExtensions;
	if (!checkDeviceExtensionSupport(potentialDevice)) {
		std::cout << "Device missing VK_KHR_SWAPCHAIN_EXTENSION_NAME" << std::endl;
		return false;
	}
	score += 1;

	if (caps.supportsDescriptorIndexing) {
		std::cout << "Device supports descriptor indexing features" << std::endl;
		requiredExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
		requiredExtensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
	}
	else {
		std::cout << "Device does NOT support descriptor indexing features" << std::endl;
	}

	deviceExtensions = requiredExtensions;
	if (!checkDeviceExtensionSupport(potentialDevice)) {
		std::cout << "Device missing required extensions for indexing" << std::endl;
		return false;
	}

	if (caps.supportsDescriptorIndexing) {
		score += 1;
	}

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(potentialDevice, &props);

	if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1;
	}

	score += props.limits.maxImageDimension2D / 1024;

	if (score > currentGreatestDeviceScore) {
		currentGreatestDeviceScore = score;
		deviceCaps = caps;
		return true;
	}

	return false;
}


// -- Logger functions -- 
void Devices::logPhysicalDevice() const {
	//log device properties and name
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;

	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

	printf("Found Physical Device:\n  Name: %s\n  Type: %d\n",
		deviceProperties.deviceName, deviceProperties.deviceType);

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
	printf("  Supports Bindless: %s\n",
		deviceCaps.supportsDescriptorIndexing ? "Yes" : "No");
};