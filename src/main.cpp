#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

//-- Main Classes --
#include "../include/VulkanInstance.h"
#include "../include/VulkanDevices.h"
#include "../include/GraphicsPipeline.h"
#include "../include/Swapchain.h"

// -- Utility Classes (customized features or general helpers) --
#include "../include/SwapchainRecreater.h"
#include "../include/MeshManager.h"
#include "../include/BufferManager.h"
 #include "../include/UniformBufferManager.h"

// == Helper functions == 
template <typename T>
void cleanDeadBuffersVerbose(std::unordered_map<std::string, std::shared_ptr<T>>& map) {
    for (auto it = map.begin(); it != map.end(); ) {
        if (!it->second) {
            std::cout << "Removing dead buffer: " << it->first << std::endl;
            it = map.erase(it);
        }
        else {
            ++it;
        }
    }
}

//Loader functions
void loadMeshesToVertexBufferManager(const MeshManager& meshManager, BufferManager& bufferManager) {
    for (const auto& meshPair : meshManager.getAllMeshes()) {
        const auto& name = meshPair.first;
        std::cout << "Loading mesh: " << static_cast <std::string> (name) << " to buffer manager : " << std::endl;

        const auto& meshPtr = meshPair.second;
        std::vector < Vertex > vertices = meshPtr->getVertices();
        std::vector < uint32_t > indices = meshPtr->getIndices();
        VkDeviceSize verticesSize = sizeof(vertices[0]) * vertices.size();
        VkDeviceSize indicesSize = sizeof(indices[0]) * indices.size();

        //Create host visible staging buffer 
        bufferManager.createBuffer(
            BufferType::VERTEX_STAGING,
            "v_staging_" + name,
            verticesSize, 
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            vertices
        );

        std::shared_ptr<Buffer> vertexStagingBuffer = bufferManager.getBuffer("v_staging_" + name);

        if (!vertexStagingBuffer) {
            std::cerr << "Error: Staging buffer creation failed for mesh: " << name << std::endl;
            continue; // Skip this mesh if the staging buffer creation failed
        }

        //Create actual vertex buffer -> runs on GPU
        bufferManager.createBuffer(
            BufferType::VERTEX,
            name,
            verticesSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            vertices
        );

        std::shared_ptr<Buffer> vertexBuffer = bufferManager.getBuffer(name);

        if (!vertexBuffer) {
            std::cerr << "Error: Vertex buffer creation failed for mesh: " << name << std::endl;
            continue; // Skip this mesh if the vertex buffer creation failed
        }

        //Check for errors
        if (vertexStagingBuffer && vertexStagingBuffer->hasErrors()) {
            vertexStagingBuffer->printErrors();
        }

        if (vertexBuffer && vertexBuffer->hasErrors()) {
            vertexBuffer->printErrors();
        }

        //Copy data from staging buffer to vertex buffer
        if (vertexStagingBuffer && vertexBuffer) {
            bufferManager.copyBuffer(vertexStagingBuffer, vertexBuffer, verticesSize);
        };

        //Now we repeat the process to load mesh index data to a index buffer
        bufferManager.createBuffer(
            BufferType::INDEX_STAGING,
            "i_staging_" + name,
            indicesSize, 
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            std::nullopt,
            indices
        );

        std::shared_ptr<Buffer> indexStagingBuffer = bufferManager.getBuffer("i_staging_" + name);

        bufferManager.createBuffer(
            BufferType::INDEX,
            "index_" + name,
            indicesSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            std::nullopt,
            indices
        );

        std::shared_ptr<Buffer> indexBuffer = bufferManager.getBuffer("index_" + name);

        //Check for errors
        if (indexStagingBuffer && indexStagingBuffer->hasErrors()) {
            indexStagingBuffer->printErrors();
        }

        if (indexBuffer && indexBuffer->hasErrors()) {
            indexBuffer->printErrors();
        }

        //Copy data from staging buffer to vertex buffer
        if (indexStagingBuffer && indexBuffer) {
            bufferManager.copyBuffer(indexStagingBuffer, indexBuffer, indicesSize);
        };
    }
};

class Application {
public: void run() {
    //Initialize Vulkan
    initVulkan();
    mainLoop();
    cleanup();
};

private:
    // -- PRIMARY CLASSES (CORE VULKAN COMPONENTS) -- 
    //Handles instance creation and validation layers
    std::shared_ptr < VulkanInstance > instance;
    //Handles physical, logical and graphicsQueue creation
    std::shared_ptr < Devices > devices;
    //Handles VkSurface and VkSwapchainKHR
    std::shared_ptr < Swapchain > swapchain;
    //Handles renderpass and pipeline/pipeline layout
    std::shared_ptr < GraphicsPipeline > graphicsPipeline;

    // -- UTILITY CLASSES --
    //Creates the swapchain recreater 
    std::shared_ptr < SwapchainRecreater > swapchainRecreater;
    //Creates a mesh manager
    std::shared_ptr < MeshManager > meshManager;
    //Creates a buffer manager
    std::shared_ptr < BufferManager > bufferManager;
    //Creates a uniform buffer manager (change to descriptor manager later)
    std::shared_ptr < UniformBufferManager > uniformBufferManager;

    void initVulkan() {
        //TODO: FIX CLEANUP -> ENSURE ALL FUNCTIONS ONLY CLEAN UP THEIR OWN RESOURCES

        //Create window, instance and debug messenger
        instance = std::make_shared <VulkanInstance>();
        instance->createInstance();
        instance->setupDebugMessenger();
        instance->createSurface();

        //Create physical and logical device
        //properly refactored to take injected classes
        devices = std::make_shared <Devices>(instance);
        devices->pickPhysicalDevice();
        devices->createLogicalDevice();

        //Create the components of the graphics pipeline
        VkPhysicalDevice physicalDevice = devices->getPhysicalDevice();
        VkDevice logicalDevice = devices->getLogicalDevice();

        meshManager = std::make_shared<MeshManager>();

        //Create VkSurface -> create surface in instance instead since its needed by swapchain and devices
        swapchain = std::make_shared<Swapchain>(instance, devices);

        //Create swapchain
        swapchain->createSwapchain();
        //Create image views
        swapchain->createImageViews();

        graphicsPipeline = std::make_shared<GraphicsPipeline>(instance, devices, swapchain);
        
        //Create descriptor set layout
        graphicsPipeline->createDescriptorSetLayout();

        //Create render pass 
        graphicsPipeline->createRenderPass();
        VkRenderPass renderPass = graphicsPipeline->getRenderPass();

        //Create actual pipeline
        graphicsPipeline->createGraphicsPipeline(uniformBufferManager);

        //Load all the swapchain recreate functions to the swapchain recreater
        swapchainRecreater = std::make_shared < SwapchainRecreater >();
        swapchainRecreater->setCallbacks(
            std::bind(&Swapchain::cleanup, swapchain.get()),
            std::bind(&Swapchain::createSwapchain, swapchain.get()),
            std::bind(&Swapchain::createImageViews, swapchain.get()),
            std::bind(&Swapchain::createSwapFramebuffers, swapchain.get(), renderPass)
        );

        swapchain->createSwapFramebuffers(renderPass);

        //Create command pool and all buffers
        graphicsPipeline->createCommandPool();

        // -> the vertex manager parameter passes in a vertex manager class, which binds vertex data to pipeline
        meshManager->addMesh(std::make_shared < TriangleMesh >());
        bufferManager = std::make_shared < BufferManager >(logicalDevice, physicalDevice,
            graphicsPipeline->getCommandPool(), devices->getGraphicsQueue());
        //Create uniform buffer manager
        uniformBufferManager = std::make_shared<UniformBufferManager>(logicalDevice, physicalDevice, graphicsPipeline->getDescriptorSetLayout(), bufferManager);

        loadMeshesToVertexBufferManager(*meshManager, *bufferManager);

        uniformBufferManager->createUniformBuffers();
        uniformBufferManager->createDescriptorPool();
        uniformBufferManager->createDescriptorSets();

        graphicsPipeline->createCommandBuffer();

        //Create sync objects 
        graphicsPipeline->createSyncObjects();
    };

    void mainLoop() {
        GLFWwindow* window = instance->getWindowPtr();

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            //Draws a frame
            graphicsPipeline->drawFrame(
                window,
                instance->framebufferResized,
                bufferManager.get(),
                swapchainRecreater.get(),
                uniformBufferManager
            );
        }
    };

    void cleanup() {
        vkDeviceWaitIdle(devices->getLogicalDevice());

        VkInstance vulkanInstance = instance->getInstance();

        // CLEANUP IN REVERSE ORDER OF CREATION
        graphicsPipeline->cleanup(vulkanInstance);
        graphicsPipeline.reset();

        bufferManager.reset();
        meshManager.reset();

        swapchain->cleanup();
        swapchain.reset();

        swapchainRecreater.reset();

        devices->cleanup();
        devices.reset();

        instance->cleanup();
        instance.reset();
    };
};

int main() {
    Application app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
};