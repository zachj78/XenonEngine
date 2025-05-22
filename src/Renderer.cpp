#include "../include/Renderer.h"
#include <tiny_obj_loader.h>
#include "../include/IO.h"

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
#include "../include/ImageManager.h"
#include "../include/ThreadPool.h"
#include "../include/DebugManager.h"

//Resources
#include "../include/Buffer.h"
#include "../include/Image.h"

#include "../include/Camera.h"

//Creates a renderer along with all it's components
void Renderer::createRenderer() {
    initCamera();
    initInstance();
    initDevices();

    initSwapchain();
    initGraphicsPipeline();

    initBufferManager();
    initImageManager();
    
    initUniformBuffer();

    initFramebufferAndSwapchain();

    initCommandBuffers();
    initSyncObjects();
};

//Runs the draw loop
void Renderer::draw() {
    //Keep track of current frame

    GLFWwindow* window = instance->getWindowPtr();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        //Update keybinds
        io->pollKeyBinds();
        
        //Apply object transformations
        uint32_t currentFrame = graphicsPipeline->getCurrentFrame();

        graphicsPipeline->drawFrame(
            window,
            instance->framebufferResized,
            descriptorManager,
            bufferManager,
            meshManager,
            swapchainRecreater
        );
    }
}

void Renderer::cleanup() {
    vkDeviceWaitIdle(devices->getLogicalDevice());

    // CLEANUP IN REVERSE ORDER OF CREATION
    cleanupResources();

    graphicsPipeline->cleanup();
    graphicsPipeline.reset();

    swapchain->cleanup();
    swapchain.reset();

    swapchainRecreater.reset();

    devices->cleanup();
    devices.reset();

    debugManager->cleanup(instance->getInstance());
    instance->cleanup();
    instance.reset();
}

//Cleans up user allocated resources, like buffers and images - then signals to destroy managers and core components
void Renderer::cleanupResources() {
    descriptorManager->cleanup();
    imageManager->cleanup();
    bufferManager->cleanup();
    meshManager.reset();
}

void Renderer::initInstance() {
    //Create window, instance and debug messenger
    debugManager = std::make_shared<DebugManager>();
    instance = std::make_shared <VulkanInstance>(debugManager);
    instance->createInstance();
    debugManager->setupDebugMessenger(instance->getInstance());
    instance->createSurface();

    io = std::make_shared<IO>(camera);
    io->setStandardBinds();
    glfwSetKeyCallback(instance->getWindowPtr(), IO::keyCallback);
    glfwSetCursorPosCallback(instance->getWindowPtr(), IO::mouseCallback);
};

void Renderer::initDevices() {
    devices = std::make_shared <Devices>(instance, debugManager);
    devices->pickPhysicalDevice();
    devices->createLogicalDevice();
};

void Renderer::initSwapchain() {
    std::cout << "creating swapchain" << std::endl;
    swapchain = std::make_shared<Swapchain>(instance, devices);
    swapchain->createSwapchain();
    swapchain->createImageViews();
}

void Renderer::initGraphicsPipeline() {
    graphicsPipeline = std::make_shared<GraphicsPipeline>(instance, devices, swapchain);
    graphicsPipeline->createRenderPass();
}

void Renderer::initBufferManager() {
    bufferManager = std::make_shared<BufferManager>(
        devices->getLogicalDevice(),
        devices->getPhysicalDevice(),
        graphicsPipeline,
        devices->getGraphicsQueue()
    );
}

void Renderer::initImageManager() {
    std::cout << "Entering initImageManager" << std::endl;
    imageManager = std::make_shared<ImageManager>(
        devices->getLogicalDevice(),
        devices->getPhysicalDevice(),
        swapchain,
        bufferManager
    );
    imageManager->createDepthImage();
}

void Renderer::initUniformBuffer() {
    std::cout << "Entering initUniformBuffer" << std::endl;
    descriptorManager = std::make_shared<DescriptorManager>(
        devices->getLogicalDevice(),
        devices->getPhysicalDevice(),
        bufferManager, 
        camera
    );
}

void Renderer::initCamera() {
    camera = std::make_shared<Camera>(glm::vec3(0, 0, 3), glm::vec3(0, 1, 0), -90.0f, 0.0f);
}

void Renderer::initFramebufferAndSwapchain() {
    std::cout << "Entering initFramebufferAndSwapchain" << std::endl;
    // Setup swapchain recreater
    swapchainRecreater = std::make_shared<SwapchainRecreater>();
    swapchainRecreater->setCallbacks(
        std::bind(&Swapchain::cleanup, swapchain),
        std::bind(&Swapchain::createSwapchain, swapchain),
        std::bind(&Swapchain::createImageViews, swapchain),
        [=]() {
            swapchain->createSwapFramebuffers(
                graphicsPipeline->getRenderPass(),
                imageManager->getDepthImageView()
            );
        },
        std::bind(&ImageManager::cleanupDepthResources, imageManager),
        std::bind(&ImageManager::createDepthImage, imageManager)
    );

    // Create framebuffers
    swapchain->createSwapFramebuffers(
        graphicsPipeline->getRenderPass(),
        imageManager->getDepthImageView()
    );
}

void Renderer::initCommandBuffers() {
    std::cout << "Entering initCommandBuffers" << std::endl;

    graphicsPipeline->createCommandPool();

    meshManager = std::make_shared<MeshManager>(devices->getLogicalDevice(), bufferManager);

    //Creates meshes and materials
    createMeshesAndMaterials();

    //Load meshes into respective buffers
    loadMeshesToVertexBufferManager();

    createDescriptorResources();

    //Pass in descriptors sets
    // set 0 -> from uniformBufferManager
    // set 1->from material manager
    // set 2->from mesh manager
    std::array<VkDescriptorSetLayout, 3> setLayouts = {
        descriptorManager->getDescriptorSetLayout(),
        meshManager->getMeshDescriptorSetLayout(),
        meshManager->getMaterialDescriptorSetLayout()
    };

    graphicsPipeline->createGraphicsPipeline(setLayouts);

    graphicsPipeline->createCommandBuffer();
};

void Renderer::createMeshesAndMaterials() {
    std::cout << "Entering createMeshesAndMaterials" << std::endl;

    imageManager->createTextureImage("tex1", "resources/textures/viking_room.png");
    std::shared_ptr<Image> texImage1 = imageManager->getImage("tex1");
    std::shared_ptr<Material> mat1 = meshManager->createMaterial("tex1", texImage1);
    
    std::shared_ptr<Mesh> plane = std::make_shared<Plane>(mat1);

    meshManager->addMesh(plane);

    //Create storage buffer AFTER materials and meshes have been added
    meshManager->createStorageBuffer();
}

void Renderer::createDescriptorResources() {
    //Get mesh and material count
    int materialCount = meshManager->getAllMaterials().size();
    int meshCount = meshManager->getAllMeshes().size();

    //Create global descriptors
    descriptorManager->createPerFrameDescriptorSetLayout();
    descriptorManager->createUniformBuffers();
    descriptorManager->createDescriptorPool(meshCount, materialCount);
    descriptorManager->createPerFrameDescriptorSet();
    
    //Now create descriptors for mesh SSBO and each material
    meshManager->createMeshDescriptorSetLayout();
    meshManager->createMaterialDescriptorSetLayout();

    meshManager->createSSBODescriptorSet(descriptorManager->getDescriptorPool());
    meshManager->createMaterialDescriptorSets(descriptorManager->getDescriptorPool());
}

void Renderer::initSyncObjects() {
    graphicsPipeline->createSyncObjects();
}

//Creates a vertex buffer and index buffer for each mesh
void Renderer::loadMeshesToVertexBufferManager() {
    for (const auto& meshPair : meshManager->getAllMeshes()) {
        const auto& name = meshPair.first;
        std::cout << "Loading mesh: " << static_cast <std::string> (name) << " to buffer manager : " << std::endl;

        const auto& meshPtr = meshPair.second;
        std::vector < Vertex > vertices = meshPtr->getVertices();
        std::vector < uint32_t > indices = meshPtr->getIndices();
        VkDeviceSize verticesSize = sizeof(vertices[0]) * vertices.size();
        VkDeviceSize indicesSize = sizeof(indices[0]) * indices.size();

        //Create host visible staging buffer 
        bufferManager->createBuffer(
            BufferType::VERTEX_STAGING,
            "v_staging_" + name,
            verticesSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            vertices
        );

        std::shared_ptr<Buffer> vertexStagingBuffer = bufferManager->getBuffer("v_staging_" + name);

        if (!vertexStagingBuffer) {
            std::cerr << "Error: Staging buffer creation failed for mesh: " << name << std::endl;
            continue; // Skip this mesh if the staging buffer creation failed
        }

        //Create actual vertex buffer -> runs on GPU
        bufferManager->createBuffer(
            BufferType::VERTEX,
            name,
            verticesSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            vertices
        );

        std::shared_ptr<Buffer> vertexBuffer = bufferManager->getBuffer(name);

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
            bufferManager->copyBuffer(vertexStagingBuffer, vertexBuffer, verticesSize);
        };

        //Now we delete the staging buffer
        vertexStagingBuffer->cleanup();
        if (vertexStagingBuffer->getHandle() == VK_NULL_HANDLE) {
            bufferManager->removeBufferByName("v_staging_" + name);
        } else {
            std::cout << "**UNABLE TO RESET VERTEX STAGING BUFFER POINTER" << std::endl;
        };

        //Now we repeat the process to load mesh index data to a index buffer
        bufferManager->createBuffer(
            BufferType::INDEX_STAGING,
            "i_staging_" + name,
            indicesSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            std::nullopt,
            indices
        );

        std::shared_ptr<Buffer> indexStagingBuffer = bufferManager->getBuffer("i_staging_" + name);

        bufferManager->createBuffer(
            BufferType::INDEX,
            "index_" + name,
            indicesSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            std::nullopt,
            indices
        );

        std::shared_ptr<Buffer> indexBuffer = bufferManager->getBuffer("index_" + name);

        //Check for errors
        if (indexStagingBuffer && indexStagingBuffer->hasErrors()) {
            indexStagingBuffer->printErrors();
        }

        if (indexBuffer && indexBuffer->hasErrors()) {
            indexBuffer->printErrors();
        }

        //Copy data from staging buffer to vertex buffer
        if (indexStagingBuffer && indexBuffer) {
            bufferManager->copyBuffer(indexStagingBuffer, indexBuffer, indicesSize);
        };

        //Delete index staging buffer
        indexStagingBuffer->cleanup();
        if (indexStagingBuffer->getHandle() == VK_NULL_HANDLE) {
            bufferManager->removeBufferByName("i_staging_" + name);
        } else {
            std::cout << "**UNABLE TO RESET INDEX STAGING BUFFER TO NULLPTR" << std::endl;
        };
    }
};