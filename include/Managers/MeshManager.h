#pragma once
#ifndef MESH_MANAGER_H
#define MESH_MANAGER_H

#include "Utils/config.h"

#include "Core/VulkanDevices.h"

#include "Managers/BufferManager.h"
#include "Managers/ImageManager.h"
#include "Managers/Image.h"
#include "Managers/Vertex.h"
#include "Managers/Material.h"

#include "Builders/DescriptorBuilder.h"

class Buffer; 

union PipelineKey {
    struct {
        uint32_t shaderID       : 10; // support up to 1024 shaders
        uint32_t blendMode      : 3;  // additive, alpha, opaque, etc.
        uint32_t cullMode       : 2;  // none, back, front
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

class Primitive {
public:

    Primitive(
        std::vector<Vertex> vertices,
        std::vector<uint32_t> indices,
        std::shared_ptr<Material> material,
        uint16_t blendModeID,
        uint16_t cullModeID,
        uint16_t depthTestID,
        uint16_t depthWriteID,
        uint16_t topologyTypeID,
        int primitiveIndex
    ) : vertices(std::move(vertices)),
        indices(std::move(indices)),
        material(material),
        primitiveIndex(primitiveIndex)
    {
        registerPipelineKey(
            blendModeID,
            cullModeID,
            depthTestID,
            depthWriteID,
            topologyTypeID
        );
    }

    //Getters
    const std::vector<Vertex>& getVertices() const {
        return vertices;
    }

    const std::vector<uint32_t>& getIndices() const {
        return indices;
    }

    const std::shared_ptr<Material>& getMaterial() const {
        return material;
    }

    const int getPrimitiveIndex() const { return primitiveIndex; };

    const int getParentMeshIndex() const { return parentMeshIndex; };

    const std::string& getParentMeshName() const { return parentMeshName; };

    // Setters/Registry
    void registerPipelineKey(
        uint16_t blendModeID,
        uint16_t cullModeID, 
        uint16_t depthTestID, 
        uint16_t depthWriteID, 
        uint16_t topologyTypeID
    ) {
        PipelineKey key{};

        std::shared_ptr<ShaderSet> materialShaderSet = material->getShaderSet();

        key.shaderID = 0; //Default

        // Shader pair of material, can be set to default for now
        key.blendMode = blendModeID; // Disabled
        key.cullMode = cullModeID; // Back cull
        key.depthTest = depthTestID; // On
        key.depthWrite = depthWriteID;
        key.topology = topologyTypeID; // Triangle(default)
        key.vertexLayoutID = 0; // default (pos, color, texcoord, tangent, normal)

        meshPipelineKey = key;
    }

    void setParentMeshIndex(int meshIndex) {
        parentMeshIndex = meshIndex; 
    }

    void setParentMeshName(std::string name) {
        parentMeshName = name;
    }

    const PipelineKey& getPipelineKey() const {
        return meshPipelineKey;
    }

private:
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    std::shared_ptr<Material> material;

    PipelineKey meshPipelineKey;

    int parentMeshIndex; 
    std::string parentMeshName; // used to access mesh transform and index from draw loop

    int primitiveIndex; // used to name v + i buffers and access them from the draw loop
};

class Mesh {
public:
    Mesh(
        std::string name, 
        std::vector<std::shared_ptr<Primitive>> primitives
    ) : name(name), primitives(primitives) {
        std::cout << "Creating Mesh " << name << std::endl;
        for (auto& primitive : primitives) {
            primitive->setParentMeshName(name);
        }
    }

    // Getters
    glm::mat4 getModelMatrix() const {
        return model;
    }

    int getMeshIndex() const {
        return meshIndex;
    }

    void setMeshIndex(int index) {
        meshIndex = index;

        for (const auto& primitive : primitives) {
            primitive->setParentMeshIndex(this->meshIndex);
        }
    }

    void setModelMatrix(glm::mat4 newMatrix) {
        model = newMatrix; 
    }

    std::string getName() const {
        return name;
    }

    std::string name; 
    std::vector<std::shared_ptr<Primitive>> getPrimitives() { return primitives; };

private: 
    std::vector<std::shared_ptr<Primitive>> primitives;

    glm::mat4 model = glm::mat4(1.0f);
    int meshIndex;
};

class MeshManager {
public:
    MeshManager(
        VkDevice logicalDevice, 
        VkPhysicalDevice physicalDevice, 
        std::shared_ptr<BufferManager> bufferManager
    );

    //Model loading functions
    void loadModel_obj(
        std::string filepath, 
        std::string name, 
        std::string materialName
    );
    
    void loadModel_gLTF(
        std::shared_ptr<BufferManager> bufferManager, 
        std::shared_ptr<ImageManager> imageManager, 
        VkDescriptorSetLayout descriptorSetLayout, 
        VkCommandPool commandPool, 
        std::string filepath, 
        std::string name
    );

    //Creates a material -> needed to bind to mesh
    void createMaterial(
        std::string name,
        std::shared_ptr<Image> textureImage
    );

    std::shared_ptr<Primitive> createPrimitive(
        std::vector<Vertex> vertices,
        std::vector<uint32_t> indices,
        std::shared_ptr<Material> material,
        uint16_t blendModeID,
        uint16_t cullModeID,
        uint16_t depthTestID,
        uint16_t depthWriteID,
        uint16_t topologyTypeID
    );

    //Creates a mesh -> material is needed
    std::shared_ptr<Mesh> createMesh(
        std::string meshName, 
        std::vector<std::shared_ptr<Primitive>> primitives
    );

    //Model matrix transform method
    void transform(
        std::string meshName, 
        std::string transformType, 
        uint32_t currentImage
    );

    //SSBO Descriptor Set up
    void createStorageBuffers();
    void createSSBODescriptors(VkDescriptorPool descriptorPool);

    //Material Descriptor Set up
    void createMaterialDescriptors(
        VkDescriptorPool descriptorPool, 
        Capabilities &deviceCaps
    );

    //For loading meshes during rendering
    void queueMeshLoad(std::shared_ptr<Mesh> mesh) {
        std::lock_guard <std::mutex> lock(meshQueueMutex);
        meshLoadQueue.push(mesh);
    }

    // == GETTERS == 
    //Storage buffer set
    const std::unordered_map<std::string, std::shared_ptr<Mesh>>& getAllMeshes() const;
    const std::unordered_map<std::string, std::shared_ptr<Material>>& getAllMaterials() const { return materials; };
    const std::vector<std::shared_ptr<Primitive>>& getAllPrimitives() const { return primitives; };
    //[THIS GETTER IS FOR DEBUGGING, NORMALLY ACCESS MATRICES THROUGH MESH POINTER]
    const std::vector<glm::mat4>& getAllModelMatrices() const { return modelMatrices; };
    const std::unordered_map<PipelineKey, std::vector<std::shared_ptr<Primitive>>>& getPrimitiveByPipelineKey() const { return primitivesByPipelineKey; };
    const std::unordered_set<PipelineKey>& getPipelineKeys() const { return uniquePipelineKeys; }

    //Sets 
    const std::vector<VkDescriptorSet>& getSSBODescriptorSets() const { return meshDescriptorSets; };

    //Set layouts
    const VkDescriptorSetLayout& getMeshDescriptorSetLayout() const { return meshDescriptorSetLayout; };
    const VkDescriptorSetLayout& getMaterialDescriptorSetLayout() const { return materialDescriptorSetLayout; };
    const std::vector<VkDescriptorSet>& getMaterialDescriptorSets() const { return materialDescriptorSets; };

    std::shared_ptr<Mesh> getMesh(const std::string& name) const {
        auto it = meshes.find(name);
        if (it != meshes.end()) return it->second;
        return nullptr;
    };

    const std::shared_ptr<Material> getMaterial(const std::string& name) const {
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

    //Stores all primitives by load order index - used to create indexed v+i buffers
    std::vector<std::shared_ptr<Primitive>> primitives;

    //Stores all materials by name
    std::unordered_map<std::string, std::shared_ptr<Material>> materials;

    //Stores <name, meshIndex> -> meshIndex is index of particular mesh in 'meshes' map
    std::unordered_map<std::string, int> meshIndices;

    //Stores primitives by pipeline key for batched rendering
    std::unordered_map<PipelineKey, std::vector<std::shared_ptr<Primitive>>> primitivesByPipelineKey;
    //Stores all unqiue pipeline keys, required to pass to graphicsPipeline module
    std::unordered_set<PipelineKey> uniquePipelineKeys;

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
};

#endif