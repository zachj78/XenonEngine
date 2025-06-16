#include "../include/Core/VulkanInstance.h"

// -- CONSTANT DECLARATION -- 
const uint32_t WIDTH = 800; 
const uint32_t HEIGHT = 600;

//Window resize callback
static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<VulkanInstance*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

// -- CLASS FUNCTIONS -- 
//Create Window on class construction
VulkanInstance::VulkanInstance(std::shared_ptr<DebugManager> debugManager) : instance_debugManager(debugManager) {
    glfwInit();

    //Set window hints
    // def: glfwWindowHint(int hint, int value)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Tell GLFW not to create OpenGL context
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); // Set the window to be non-resizable

    // Create actual window object
    // def: glfwCreateWindow(int width, int height, const char* title, GLFWmonitor* monitor, GlfWwindow* share)
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Engine", nullptr, nullptr);

    glfwSetWindowUserPointer(window, this);
    
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
};

void VulkanInstance::createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create a window surface");
    } else {
        std::cout << "Window surface created successfully" << std::endl;
    };
};

//Destroy window and VkInstance on class cleanup
void VulkanInstance::cleanup() {
    std::cout << "    Destroying `Instance` " << std::endl;

    vkDestroySurfaceKHR(instance, surface, nullptr);

    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
};

//This function creates a VkInstance variable
// -> this acts as a connection between app and api
void VulkanInstance::createInstance() {
    //Before we create an instance, we should check that our validation layers are supported
    if (enableValidationLayers && instance_debugManager->checkValidationLayerSupport() == false) {
        throw std::runtime_error("Validation layers unavailable");
    };

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Test Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    uint32_t apiVersion = 0;
    VkResult result = vkEnumerateInstanceVersion(&apiVersion);

    appInfo.apiVersion = apiVersion;

    uint32_t major = VK_VERSION_MAJOR(apiVersion);
    uint32_t minor = VK_VERSION_MINOR(apiVersion);
    uint32_t patch = VK_VERSION_PATCH(apiVersion);

    std::cout << "Vulkan Version Supported: " << major << "." << minor << "." << patch << std::endl;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (enableValidationLayers) {
        //If we have validation layers on we need to pass our validationLayers struct data
        const auto& validationLayers = instance_debugManager->getValidationLayers();

        for (auto layer : validationLayers) {
            std::cout << "Validation layer: " << layer << std::endl;
        }

        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        // we also need to pass a debug messenger create info struct to pNext,
        // this allows the createInstance function to be debugged, as otherwise
        // the debug messenger will be destroyed before the instance & created after
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        instance_debugManager->populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr; 
    };

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    };
};