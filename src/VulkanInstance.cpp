#include "../include/VulkanInstance.h"

// -- CONSTANT DECLARATION -- 
const uint32_t WIDTH = 800; 
const uint32_t HEIGHT = 600;

// -- HELPER FUNCTIONS -- 
bool checkValidationLayerSupport(std::vector<const char*> validationLayers) {
    //Get all possible VkInstance layer count
    uint32_t layerCount; 
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    //Creates vec availableLayers of size layerCount 
    std::vector<VkLayerProperties> availableLayers(layerCount);
    //Assign instance layers to availableLayers
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // For each layer in our validation layer struct, 
    // it will be checked that it exists in availableLayers
    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break; 
            }
        };

        if (!layerFound) {
            return false;
        }
    };

    return true;
};

//Debug messenger callback function
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    //Error message severity
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    //Error message type
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    //CallbackData struct -> contains pMessage, pObjects and objectCount
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
};

//This function loads and executes the vkCreateDebugUtilsMessengerEXT function
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger
) {
    auto func = LoadInstanceFunction<PFN_vkCreateDebugUtilsMessengerEXT>(instance, "vkCreateDebugUtilsMessengerEXT");
    
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
};

//This function loads and executes the vkDestroyDebugUtilsMessengerEXT function
void DestroyDebugUtilsMessengerEXT(VkInstance instance, 
    VkDebugUtilsMessengerEXT debugMessenger, 
    const VkAllocationCallbacks* pAllocator
) {
    auto func = LoadInstanceFunction<PFN_vkDestroyDebugUtilsMessengerEXT>(instance, "vkDestroyDebugUtilsMessengerEXT");

    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    };
};

//Populates a createInfo struct for a debug utility messenger
void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

// -- CLASS FUNCTIONS -- 
//Create Window on class construction
VulkanInstance::VulkanInstance() {
    glfwInit();

    //Set window hints
    // def: glfwWindowHint(int hint, int value)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Tell GLFW not to create OpenGL context
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // Set the window to be non-resizable

    // Create actual window object
    // def: glfwCreateWindow(int width, int height, const char* title, GLFWmonitor* monitor, GlfWwindow* share)
    window = glfwCreateWindow(WIDTH, HEIGHT, "Application Running...", nullptr, nullptr);
};

//Destroy window and VkInstance on class cleanup
void VulkanInstance::cleanup() {
    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    };

    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
};

//This function creates a VkInstance variable
// -> this acts as a connection between app and api
void VulkanInstance::createInstance() {
    //Before we create an instance, we should check that our validation layers are supported
    if (enableValidationLayers && !checkValidationLayerSupport(validationLayers)) {
        throw std::runtime_error("Validation layers unavailable");
    };

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Test Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (enableValidationLayers) {
        //If we have validation layers on we need to pass our validationLayers struct data
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        // we also need to pass a debug messenger create info struct to pNext,
        // this allows the createInstance function to be debugged, as otherwise
        // the debug messenger will be destroyed before the instance & created after
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr; 
    };

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    };
};

//This function creates our debug messenger(VulkanInstance::debugMessenger)
// -> this debug messenger is responsible for initiating the callback function 
// -> on validation layer error
void VulkanInstance::setupDebugMessenger() {
    // If we are not in debug -> messenger will not be created
    if (!enableValidationLayers) return; 

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create debug messenger");
    }
};