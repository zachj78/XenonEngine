#pragma once
#ifndef MESH_MANAGER_H
#define MESH_MANAGER_H

#include "Utils/config.h"

#include "Core/VulkanDevices.h"

#include "Managers/BufferManager.h"
#include "Managers/Image.h"
#include "Managers/Vertex.h"
#include "Managers/Material.h"

#include "Builders/DescriptorBuilder.h"

class Buffer; 

union PipelineKey {
    struct {
        uint32_t shaderID       : 10; // support up to 1024 shaders
        uint32_t blendMode      : 3;  // additive, alpha, opaque, etc.
        uint32_t cullMode       : 2;  // none, front, back
        uint32_t depthTest      : 1;  // on/off
        uint32_t depthWrite     : 1;
        uint32_t topology       : 3;  // triangle, line, etc.
        uint32_t vertexLayoutID : 8;  // compact ID from vertex attributes(pos, color, texcoord, etc)
        uint32_t padding        : 4;  // Pad to 32 bits
    };
    uint32_t packed; 

    PipelineKey() : packed(0) {}

    bool operator==(const PipelineKey& other) const {
        return packed == other.packed;
    }
};

// For hashing support
namespace std {
    template<>
    struct hash<PipelineKey> {
        size_t operator()(const PipelineKey& key) const {
            return std::hash<uint32_t>()(key.packed);
        }
    };
}

//Generate pipeline key per mesh, the pipeline key determines the pipeline bound at draw time
// Figure out how many different pipelines you should make, how to efficiently tag pipelines with keys

class Mesh {
public:
    virtual const std::vector<Vertex>& getVertices() const = 0;
    virtual const std::vector<uint32_t>& getIndices() const = 0;
    virtual glm::mat4 getModelMatrix() const = 0;
    virtual int getMeshIndex() const = 0;
    virtual void setMeshIndex(int index) = 0;
    virtual std::string getName() const = 0;
    virtual std::shared_ptr<Material> getMaterial() const = 0;
    virtual void registerPipelineKey(uint16_t topologyTypeID) = 0; 
    virtual const PipelineKey& getPipelineKey() const = 0; 

    virtual ~Mesh() = default;
};

class Plane : public Mesh {
public:
    Plane(std::shared_ptr<Material> material) 
        : material(material), model(glm::mat4(1.0f)), meshIndex(-1) {
        registerPipelineKey(0); // pass in default topology, 0 = TRIANGLE
    }

    const std::vector<Vertex>& getVertices() const override {
        return vertices;
    }

    const std::vector<uint32_t>& getIndices() const override {
        return indices;
    }

    glm::mat4 getModelMatrix() const override {
        return model;
    }

    int getMeshIndex() const override {
        return meshIndex;
    }

    void setMeshIndex(int index) override {
        meshIndex = index;
        std::cout << "New mesh index: " << meshIndex << std::endl;
    }

    std::string getName() const override {
        return "Plane";
    }

    std::shared_ptr<Material> getMaterial() const override {
        return material;
    }

    void registerPipelineKey(uint16_t topologyTypeID) override {
        PipelineKey key{};

        ShaderSet materialShaderSet = material->getShaderSet();

        //get an id based off this shader(idk how this will work to be 100%)
        key.shaderID = 0;

        // Shader pair of material, can be set to default for now
        key.blendMode = 0; // Disabled
        key.cullMode = 2; // Back cull
        key.depthTest = 1; // On
        key.depthWrite = 1;
        key.topology = 0; // Triangle(default)
        key.vertexLayoutID = 2; // pos, color, texcoord

        meshPipelineKey = key;
    }

    const PipelineKey& getPipelineKey() const {
        return meshPipelineKey; 
    }

private:
    glm::mat4 model;
    int meshIndex;
    std::shared_ptr<Material> material;

    const std::vector<Vertex> vertices = {
        {{-0.5f,  0.0f,  0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, 
        {{ 0.5f,  0.0f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, 
        {{ 0.5f,  0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, 
        {{-0.5f,  0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}} 
    };

    const std::vector<uint32_t> indices = {
        0, 1, 2, 2, 3, 0
    };

    PipelineKey meshPipelineKey;
};

class DynamicMesh : public Mesh {
public:

    //Each mesh needs to allow a parameter for topology
    // (and maybe later vertex attribute count or something to determine id?)
    // for now add topology
    DynamicMesh(
        std::string name, 
        std::vector<Vertex> vertices, 
        std::vector<uint32_t> indices, 
        std::shared_ptr<Material> material, 
        uint16_t topologyTypeID
    )
        : name(std::move(name)), 
        vertices(std::move(vertices)), 
        indices(std::move(indices)), 
        material(material), 
        meshIndex(-1) 
    {
        registerPipelineKey(topologyTypeID);
    }

    const std::vector<Vertex>& getVertices() const override {
        return vertices;
    }

    const std::vector<uint32_t>& getIndices() const override {
        return indices;
    }

    glm::mat4 getModelMatrix() const override {
        return model;
    }

    int getMeshIndex() const override {
        return meshIndex;
    }

    void setMeshIndex(int index) override {
        meshIndex = index;
    }

    std::string getName() const override {
        return name;
    }

    std::shared_ptr<Material> getMaterial() const override {
        return material;
    }

    void registerPipelineKey(uint16_t topologyTypeID) override {
        PipelineKey key{};

        ShaderSet materialShaderSet = material->getShaderSet();

        key.shaderID = 0; //Default unlit textured

        // Shader pair of material, can be set to default for now
        key.blendMode = 0; // Disabled
        key.cullMode = 2; // Back cull
        key.depthTest = 1; // On
        key.depthWrite = 1;
        key.topology = topologyTypeID; // Triangle(default)
        key.vertexLayoutID = 2; // pos, color, texcoord

        meshPipelineKey = key;
    }

    const PipelineKey& getPipelineKey() const {
        return meshPipelineKey;
    }

private:
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    glm::mat4 model = glm::mat4(1.0f);
    int meshIndex;
    
    std::shared_ptr<Material> material;

    PipelineKey meshPipelineKey; 
};

class MeshManager {
public:
    MeshManager(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, std::shared_ptr<BufferManager> bufferManager);

    //Model loading functions
    void loadModel_obj(std::string filepath, std::string name, std::string materialName);
    void loadModel_gLTF(std::shared_ptr<BufferManager> bufferManager, VkDescriptorSetLayout descriptorSetLayout, VkCommandPool commandPool, std::string filepath, std::string name);

    //Gets or creates a shaderID for a given shader set
    int getOrCreateShaderID(const ShaderSet) {}

    //Adds a mesh to be drawn
    void addMesh(std::shared_ptr<Mesh> mesh);

    //Creates a material -> needed to bind to mesh
    void createMaterial(std::string name,
        std::shared_ptr<Image> textureImage);

    //Creates a mesh -> material is needed
    std::shared_ptr<Mesh> createMesh(std::string meshName, std::string matName);

    //Model matrix transform method
    void transform(std::string meshName, std::string transformType, uint32_t currentImage);

    //SSBO Descriptor Set up
    void createStorageBuffers();
    void createSSBODescriptors(VkDescriptorPool descriptorPool);

    //Material Descriptor Set up
    void createMaterialDescriptors(VkDescriptorPool descriptorPool, Capabilities deviceCaps);

    //For loading meshes during rendering
    void queueMeshLoad(std::shared_ptr<Mesh> mesh) {
        std::lock_guard <std::mutex> lock(meshQueueMutex);
        meshLoadQueue.push(mesh);
    }

    // == GETTERS == 
    //Storage buffer set
    const std::unordered_map<std::string, std::shared_ptr<Mesh>>& getAllMeshes() const;
    const std::unordered_map<std::string, std::shared_ptr<Material>>& getAllMaterials() const { return materials; };
    const std::shared_ptr<Material> getMaterial(std::string materialName) {
        auto it= materials.find(materialName);

        if (it != materials.end()) {
            return it->second;
        } else {
            std::cerr << "Failed to find material by name : [" << materialName << "]" << std::endl;
        }
    }
    
    //Sets 
    std::vector<VkDescriptorSet> getSSBODescriptorSets() { return meshDescriptorSets; };

    //Set layouts
    VkDescriptorSetLayout getMeshDescriptorSetLayout() { return meshDescriptorSetLayout; };
    VkDescriptorSetLayout getMaterialDescriptorSetLayout() { return materialDescriptorSetLayout; };
    std::vector<VkDescriptorSet> getMaterialDescriptorSets() { return materialDescriptorSets; };

    std::shared_ptr<Mesh> getMesh(const std::string& name) const {
        auto it = meshes.find(name);
        if (it != meshes.end()) return it->second;
        return nullptr;
    };

    std::shared_ptr<Material> getMaterial(const std::string& name) const {
        auto it = materials.find(name);
        if (it != materials.end()) {
            return it->second;
        };
        return nullptr;
    }

private: 
    bool deviceSupportsBindless = false; 
    int meshCount;
    int textureTypes = 1;

    //Injected Vulkan logical device
    VkDevice meshManager_logicalDevice;
    VkPhysicalDevice meshManager_physicalDevice;

    //Stores all meshes by name
    std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes;
    //Stores all materials by name
    std::unordered_map<std::string, std::shared_ptr<Material>> materials;
    //Stores <name, meshIndex>
    std::unordered_map<std::string, int> meshIndices;

    //SSBO Management
    std::shared_ptr<BufferManager> meshManager_bufferManager;
    std::vector<void*> mappedStorageBufferPtrs;
    std::vector<glm::mat4> modelMatrices;

    //Hot-loading queue
    std::mutex meshQueueMutex;
    std::queue<std::shared_ptr<Mesh>> meshLoadQueue;

    //Descriptor Management
    VkDescriptorSetLayout materialDescriptorSetLayout;
    VkDescriptorSetLayout meshDescriptorSetLayout;
    std::vector<VkDescriptorSet> meshDescriptorSets;
    std::vector<VkDescriptorSet> materialDescriptorSets;

    //Shader ID map
    std::unordered_map<ShaderSet, uint16_t> shaderIDs;
    int nextShaderID = 0; 

    //Pipeline keys
    std::vector<std::shared_ptr<PipelineKey>> pipelineKeys; 
};

#endif