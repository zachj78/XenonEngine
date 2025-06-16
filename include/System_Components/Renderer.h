#pragma once
#ifndef RENDERER_H
#define RENDERER_H

#include "Utils/config.h"

// Forward declarations of main components
class VulkanInstance;
class Devices;
class RenderTargeter;
class GraphicsPipeline;

// Utility classes
class SwapchainRecreater;
class MeshManager;
class BufferManager;
class DescriptorManager;
class ImageManager;
class DebugManager;
class ThreadPool;
class Camera;
class GUI; 

// Other system components
class IO;

//Thread queue request structs
struct MaterialRequest {
    std::string name;
    std::string pathToImage;
};

struct MeshRequest {
    std::string name;
    std::string materialName;
    std::string sourcePathOrPrefabFlag;
};

class Renderer {
public:
    // Main creation function
    void createRenderer();
    // Draws a frame
    void draw();


    // == Initializer functions ==
    void initCamera();
    void initInstance();
    void initDevices();

    void initBufferManager();
    void initImageManager();

    void createRenderTargetResources();
    void initRenderpassAndCommandPool();
    void initFramebuffers(); 

    void initUniformBuffer();

    void linkImGui(); 

    void initCommandBuffers();

    void initSyncObjects();
    void createDescriptorResources();

    // MESH/MATERIAL CREATION FUNCTIONS

    void createMeshesAndMaterials();
    void createMaterial(std::string materialName, std::string pathToImage); // EXPOSED FUNCTION
    void createMesh(std::string meshName, std::string materialName, std::string filePath); // EXPOSED FUNCTION
    void loadMeshesToVertexBufferManager();

    //Submits a mesh request to the request queue
    void submitMeshRequest(const MeshRequest &request);
    void checkMeshQueue();
    void submitMaterialRequest(const MaterialRequest& request);
    void checkMaterialQueue();

    // == Cleanup == 
    void cleanup();
    void cleanupResources();

private:
    // Renderer State/Flags
    // sets game/engine state for correct window docking
    bool inGame = true; 

    // Core Vulkan components
    std::shared_ptr<VulkanInstance> instance;
    std::shared_ptr<Devices> devices;
    std::shared_ptr<RenderTargeter> renderTargeter;
    std::shared_ptr<GraphicsPipeline> graphicsPipeline;

    // Utility managers
    std::shared_ptr<SwapchainRecreater> swapchainRecreater;
    std::shared_ptr<MeshManager> meshManager;
    std::shared_ptr<BufferManager> bufferManager;
    std::shared_ptr<DescriptorManager> descriptorManager;
    std::shared_ptr<ThreadPool> threadPool; 
    std::shared_ptr<ImageManager> imageManager;
    std::shared_ptr<DebugManager> debugManager;

    //material and mesh queues
    std::queue<MaterialRequest> materialQueue;
    std::queue<MeshRequest> meshQueue;
    std::mutex queueMutex;

    // Camera setup
    std::shared_ptr<Camera> camera; 

    // Other system components
    std::shared_ptr<IO> io;
    std::shared_ptr<GUI> gui; 
};

#endif // RENDERER_H
