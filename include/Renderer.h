#pragma once
#ifndef RENDERER_H
#define RENDERER_H

#include "config.h"

// Forward declarations of main components
class VulkanInstance;
class Devices;
class Swapchain;
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

// Other system components
class IO;

class Renderer {
public:
    // Main creation function
    void createRenderer();
    // Draws a frame
    void draw();

    // == Initializer functions ==
    void initInstance();
    void initDevices();
    void initSwapchain();
    void initGraphicsPipeline();

    void initBufferManager();
    void initImageManager();
    void initUniformBuffer();

    void initCamera();

    void initFramebufferAndSwapchain();
    void initCommandBuffers();
    void initSyncObjects();
    void createDescriptorResources();

    void createMeshesAndMaterials();
    void loadMeshesToVertexBufferManager();

    // == Cleanup == 
    void cleanup();
    void cleanupResources();

private:

    // Core Vulkan components
    std::shared_ptr<VulkanInstance> instance;
    std::shared_ptr<Devices> devices;
    std::shared_ptr<Swapchain> swapchain;
    std::shared_ptr<GraphicsPipeline> graphicsPipeline;

    // Utility managers
    std::shared_ptr<SwapchainRecreater> swapchainRecreater;
    std::shared_ptr<MeshManager> meshManager;
    std::shared_ptr<BufferManager> bufferManager;
    std::shared_ptr<DescriptorManager> descriptorManager;
    std::shared_ptr<ImageManager> imageManager;
    std::shared_ptr<DebugManager> debugManager;

    // Camera setup
    std::shared_ptr<Camera> camera; 

    // Other system components
    std::shared_ptr<IO> io;
};

#endif // RENDERER_H
