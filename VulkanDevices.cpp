#include "VulkanDevices.h"

// -- Constant Declaration --

// -- Struct Declaration -- 
struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;

	bool isComplete() {
		return graphicsFamily.has_value();
	};
};

// -- Helper functions --
//This function finds indices of queue families specified in QueueFamilyIndices
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice) {
	QueueFamilyIndices indices; 

	//find number of available queue families
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	//create vec<VkQueueFamilyProperties> queueFamiles of size queueFamilyCount
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		};

		if (indices.isComplete()) {
			break;
		};

		i++;
	};

	return indices;
};

//This function checks if a given physical device is suitable
bool isDeviceSuitable(VkPhysicalDevice physicalDevice) {
	//Query for basic device properties(name, type, supported vulkan version)
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

	//Query for basic device features(texture compression, multiviewport rendering)
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

	//Query our queue familes, assign to QueueFamilyIndices
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

	// returns true for discrete gpus or integrated gpus with geometry shader capabilities
	// also ensures we have found a indice for a graphics queue family
	return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
		&& deviceFeatures.geometryShader && indices.isComplete();
};

// -- Main functions -- 
void VulkanDevices::pickPhysicalDevice(VkInstance instance) {
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
	};
};

void VulkanDevices::createLogicalDevice(const std::vector<const char*> validationLayers) {
	//First we create struct VkDeviceQueueCreateInfo
	// -> this struct specifies # of queues we want for a single queue family
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	// we are only using graphics queue family currently
	queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value(); 
	queueCreateInfo.queueCount = 1; 

	//This effects the scheduling of command buffer execution if you have multiple queueFamilies
	float queuePriority = 1.0f; 
	queueCreateInfo.pQueuePriorities = &queuePriority;
	
	//Specifies device features we will be using, 
	// -> not used now 
	VkPhysicalDeviceFeatures deviceFeatures{};

	//Main DeviceCreateInfo struct 
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.queueCreateInfoCount = 1; 
	createInfo.pEnabledFeatures = &deviceFeatures; 

	createInfo.enabledExtensionCount = 0;

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	};

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create a logical device");
	};

	//store our graphics queue family in a VkQueue handle
	vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
};

VulkanDevices::~VulkanDevices() {
	//Destroy logical device
	vkDestroyDevice(device, nullptr);
};