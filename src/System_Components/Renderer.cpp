#include "../include/System_Components/Renderer.h"
#include "../include/External/tiny_obj_loader.h"
#include "../include/System_Components/IO.h"
#include "../include/System_Components/GUI.h"

//-- Main Classes --
#include "../include/Core/VulkanInstance.h"
#include "../include/Core/VulkanDevices.h"
#include "../include/Core/GraphicsPipeline.h"
#include "../include/Core/Swapchain.h"

// -- Utility Classes (customized features or general helpers) --
#include "../include/Managers/SwapchainRecreater.h"
#include "../include/Managers/MeshManager.h"
#include "../include/Managers/BufferManager.h"
#include "../include/Managers/DescriptorManager.h"
#include "../include/Managers/ImageManager.h"
#include "../include/Utils/ThreadPool.h"
#include "../include/Managers/DebugManager.h"

//Resources
#include "../include/Managers/Buffer.h"
#include "../include/Managers/Image.h"

//Other systems
#include "../include/System_Components/ECS.h"
#include "../include/System_Components/Camera.h"

//Creates a renderer along with all it's components
void Renderer::createRenderer() {
    //RENDER PIPELINE SET UP
    initCamera();
    initInstance();
    initDevices();

    //GPU resource managers
    initBufferManager();
    initImageManager();
    initUniformBuffer();

    //Render pass, command pool, render target and framebuffers
    createRenderTargetResources();
    initRenderpassAndCommandPool();
    initFramebuffers();

    //GUI
    linkImGui();

    //Command buffers and sync primitives 
    initCommandBuffers();
    initSyncObjects();
    
};

//Runs the draw loop
void Renderer::draw() {
    //Keep track of current frame
    std::cout << "Entering draw loop: " << std::endl;

    GLFWwindow* window = instance->getWindowPtr();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        //Update keybinds
        io->pollKeyBinds();

        //Check mesh queue
        checkMaterialQueue();
        checkMeshQueue();

        //Apply object transformations
        uint32_t currentFrame = graphicsPipeline->getCurrentFrame();

        if (inGame) {
            graphicsPipeline->drawSwapchain(
                window,
                instance->framebufferResized,
                descriptorManager,
                bufferManager,
                meshManager,
                swapchainRecreater,
                gui,
                renderTargeter
            );
        } else {
            graphicsPipeline->drawOffscreen(window,
                instance->framebufferResized,
                descriptorManager,
                bufferManager,
                meshManager,
                swapchainRecreater,
                gui,
                renderTargeter);
        }
    }
}

// ================================
//    INITIALIZATION FUNCTIONS
// ================================
void Renderer::initCamera() {
    camera = std::make_shared<Camera>(glm::vec3(0, 2, 2), glm::vec3(0, 0.0f, -1.0f), -90.0f, -89.99f);
}

void Renderer::initInstance() {
    // == CREATE 
    //   -> WINDOW 
    //   -> DEBUG MESSENGER
    //   -> SURFACE
    debugManager = std::make_shared<DebugManager>();
    instance = std::make_shared <VulkanInstance>(debugManager);
    instance->createInstance();
    debugManager->setupDebugMessenger(instance->getInstance());
    instance->createSurface();

    // == CREATE IO == 
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

// ================================
//      GPU RESOURCE MANAGERS INIT
// ================================
void Renderer::initBufferManager() {
    bufferManager = std::make_shared<BufferManager>(
        devices->getLogicalDevice(),
        devices->getPhysicalDevice(),
        devices->getGraphicsQueue()
    );
}

void Renderer::initImageManager() {
    std::cout << "Entering initImageManager" << std::endl;
    imageManager = std::make_shared<ImageManager>(
        devices->getLogicalDevice(),
        devices->getPhysicalDevice(),
        bufferManager
    );
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

// ============================================
// RENDER PASS, RENDER TARGET AND FRAMEBUFFERS
// ============================================

void Renderer::createRenderTargetResources() {
    std::cout << "Entering createRenderTargetResources()" << std::endl;

    renderTargeter = std::make_shared<RenderTargeter>(instance, devices);

    //Get extent and format
    renderTargeter->getFramebufferDetails();
}

void Renderer::initRenderpassAndCommandPool() {
    std::cout << "Entering initRenderpassAndCommandPool()" << std::endl;
    graphicsPipeline = std::make_shared<GraphicsPipeline>(instance, devices);

    renderTargeter->createMainRenderpass(inGame);
    if (!inGame) {
        std::cout << "CREATING OFFSCREEN PASS" << std::endl;
        renderTargeter->createOffscreenRenderpass(); 
    }

    graphicsPipeline->createCommandPool();
}

void Renderer::initFramebuffers() {
    std::cout << "entering initFramebuffers()" << std::endl;

    // Setup swapchain recreater
    swapchainRecreater = std::make_shared<SwapchainRecreater>();
    //Create depth image
    renderTargeter->createDepthImage();

    if (inGame) {
        std::cout << "Creating SWAPCHAIN <====" << std::endl;
        //Create swapchain 
        renderTargeter->createSwapchainResources();
        renderTargeter->createFramebuffers(
            renderTargeter->getMainPass(), 
            false
        );
    }

    swapchainRecreater->setCallbacks(
        std::bind(&RenderTargeter::cleanup, renderTargeter, false),
        std::bind(&RenderTargeter::getFramebufferDetails, renderTargeter),
        std::bind(&RenderTargeter::createDepthImage, renderTargeter),
        std::bind(&RenderTargeter::createSwapchainResources, renderTargeter),
        std::bind(&RenderTargeter::createFramebuffers, renderTargeter, renderTargeter->getMainPass(), false)
    );

    if(!inGame){
        //Create offscreen target 
        std::cout << "Creating OFFSCREEN TARGET <====" << std::endl;

        //Creae BOTH main framebuffers and offscreen framebuffers
        //renderTargeter->createOffscreenResources(
        //    bufferManager,
        //    graphicsPipeline->getCommandPool()
        //);  

        renderTargeter->createFramebuffers(
            renderTargeter->getMainPass(),
            false
        );

        renderTargeter->createFramebuffers(
            renderTargeter->getOffscreenPass(),
            true
        );

        //swapchainRecreater->setCallbacks(
        //    std::bind(&RenderTargeter::cleanup, renderTargeter),
        //    std::bind(&RenderTargeter::createOffscreenResources, renderTargeter, bufferManager, graphicsPipeline->getCommandPool()),
        //    std::bind(&RenderTargeter::createFramebuffers, renderTargeter, renderTargeter->getOffscreenPass(), imageManager->getDepthImageView(), true),
        //    std::bind(&ImageManager::cleanupDepthResources, imageManager),
        //    std::bind(&ImageManager::createDepthImage, imageManager, renderTargeter->getRenderTarget().extent)
        //);
    }
}

// =========================================
// COMMAND BUFFER AND SYNC PRIMITIVES
// =========================================
void Renderer::linkImGui() {
    std::cout << "Linking GUI" << std::endl;

    gui = std::make_shared<GUI>();

    //[TESTING: THIS SHOULD BE SET FALSE, GUI WILL OVERLAY OTHERWISE]
    if (inGame) {
        gui->linkToApp(
            inGame,
            instance->getWindowPtr(),
            instance->getInstance(),
            devices->getLogicalDevice(),
            devices->getPhysicalDevice(),
            devices->getQueueFamilies().graphicsFamily.value(),
            devices->getGraphicsQueue(),
            renderTargeter->getRenderTarget().images.size(),
            renderTargeter->getRenderTarget().images.size(),
            renderTargeter->getRenderTarget().imageViews,
            renderTargeter->getMainPass() // USE MAIN PASS
        );
    };
};

void Renderer::initCommandBuffers() {
    std::cout << "Entering initCommandBuffers" << std::endl;

    meshManager = std::make_shared<MeshManager>(devices->getLogicalDevice(), devices->getPhysicalDevice(), bufferManager);

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

    graphicsPipeline->createGraphicsPipeline(renderTargeter, setLayouts);

    graphicsPipeline->createCommandBuffer();
};

void Renderer::initSyncObjects() {
    graphicsPipeline->createSyncObjects();
}

// =======================================
//  CALLED FROM W/IN initCommandBuffers()
// =======================================

// later going to be used to read meshes and materials from save files and load on first render
void Renderer::createMeshesAndMaterials() {
    std::cout << "Entering createMeshesAndMaterials" << std::endl;

    std::string materialName = "mat1";

    createMaterial(materialName, "resources/textures/viking_room.png");
    createMesh("plane1", materialName, "pf_Plane");

    //[TEST] TRYING OUT GLTF LOADING FUNCTION
    /*meshManager->loadModel_gLTF(
        bufferManager,
        descriptorManager->getDescriptorSetLayout(),
        graphicsPipeline->getCommandPool(),
        "resources/models/test.glb",
        "test"
    );*/

    std::cout << "Properly created Meshes" << std::endl;

    //Create storage buffer AFTER materials and meshes have been added
    meshManager->createStorageBuffers();
};

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
            bufferManager->copyBuffer(vertexStagingBuffer, vertexBuffer, verticesSize, graphicsPipeline->getCommandPool());
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
            bufferManager->copyBuffer(indexStagingBuffer, indexBuffer, indicesSize, graphicsPipeline->getCommandPool());
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

void Renderer::createDescriptorResources() {
    //DESCRIPTOR CREATION AND BINDING DEPENDS HEAVILY ON DEVICE COMPATABILITY
    Capabilities deviceCaps; 

    //Get mesh and material count
    int materialCount = meshManager->getAllMaterials().size();
    int meshCount = meshManager->getAllMeshes().size();

    //Create global descriptors
    descriptorManager->createUniformBuffers();
    descriptorManager->createDescriptorPool(meshCount, materialCount);
    descriptorManager->createPerFrameDescriptors();

    meshManager->createSSBODescriptors(descriptorManager->getDescriptorPool());
    meshManager->createMaterialDescriptors(descriptorManager->getDescriptorPool(), deviceCaps);
}

void Renderer::cleanup() {
    vkDeviceWaitIdle(devices->getLogicalDevice());

    // CLEANUP IN REVERSE ORDER OF CREATION
    cleanupResources();

    graphicsPipeline->cleanup();
    graphicsPipeline.reset();

    renderTargeter->cleanup(true);
    renderTargeter.reset();

    swapchainRecreater.reset();

    devices->cleanup();
    devices.reset();

    debugManager->cleanup(instance->getInstance());
    instance->cleanup();
    instance.reset();
}

// ====================================
// IN-FLIGHT MATERIAL/MESH QUEUE HANDLING
// ====================================
void Renderer::createMaterial(std::string materialName, std::string pathToImage) {
    std::string imageName = materialName + "_image";

    std::cout << "[CREATING IMAGE FOR MATERIAL] : " << imageName << std::endl;

    imageManager->createTextureImage(imageName, pathToImage, graphicsPipeline->getCommandPool());
    std::shared_ptr<Image> texImage1 = imageManager->getImage(imageName);

    std::cout << "Found image << " << texImage1->getImageDetails().imageFormat << std::endl;

    std::cout << "Properly Created Image for material : [" << materialName << "]" << std::endl;

    meshManager->createMaterial(materialName, texImage1);
}

//Prefab objects are passed in with "pf" prefix: ex: "pf_Cube"
void Renderer::createMesh(std::string meshName, std::string materialName, std::string filePath) {
    std::shared_ptr<Material> mat = meshManager->getMaterial(materialName);

    if (filePath == "pf_Plane") {
        std::shared_ptr<Mesh> plane = std::make_shared<Plane>(mat);
        meshManager->addMesh(plane);
    } else {
        std::cout << "Loading model ::" << std::endl;
    }
}

void Renderer::submitMeshRequest(const MeshRequest& request) {
    std::lock_guard<std::mutex> lock(queueMutex);
    meshQueue.push(request);
}

void Renderer::checkMeshQueue() {
    std::lock_guard<std::mutex> lock(queueMutex);

    while (!meshQueue.empty()) {
        MeshRequest request = meshQueue.front();
        meshQueue.pop();

        std::future<int> future = threadPool->submit([this, request]() {
            createMesh(request.name, request.materialName, request.sourcePathOrPrefabFlag);
            return 1;
        });
    }
}

void Renderer::submitMaterialRequest(const MaterialRequest& request) {
    std::lock_guard<std::mutex> lock(queueMutex);
    materialQueue.push(request);
}

void Renderer::checkMaterialQueue() {
    std::lock_guard<std::mutex> lock(queueMutex);

    while (!materialQueue.empty()) {
        MaterialRequest request = materialQueue.front();
        materialQueue.pop();

        //ADD BETTER ERROR CHECKING RATHER THAN JUST RETURNING 1
        std::future<int> future = threadPool->submit([this, request]() {
            createMaterial(request.name, request.pathToImage);
            return 1;
        });
    }
}

//Cleans up user allocated resources, like buffers and images - then signals to destroy managers and core components
void Renderer::cleanupResources() {
    descriptorManager->cleanup();
    imageManager->cleanup();
    bufferManager->cleanup();
    meshManager.reset();
}