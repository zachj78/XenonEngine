#pragma once
#ifndef MESH_MANAGER_H
#define MESH_MANAGER_H

#include "Config.h"
#include "BufferManager.h"
#include "Image.h"
#include "Vertex.h"

class Buffer; 

struct MeshData {
    glm::mat4 model; 
};

class Material {
public:
    Material(std::string name,
        std::shared_ptr<Image> textureImage,
        VkDescriptorSetLayout descriptorSetLayout,
        VkDevice logicalDevice
    )
        : name(name), textureImage(textureImage), descriptorSetLayout(descriptorSetLayout), logicalDevice(logicalDevice) {
        std::cout << "Created material : " << name << std::endl;
    }

    void initDescriptors(VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout) {
        createDescriptorSet(descriptorPool, descriptorSetLayout);
    }

    VkDescriptorSet getDescriptorSet() { return descriptorSet; };
    std::string getName() { return name; };

private:

    void createDescriptorSet(VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout) {
        std::cout << "Creating material descriptor set" << std::endl;

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;

        if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, &descriptorSet) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate descriptor sets");
        };

        //Create image info for texture sampler
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImage->getImageDetails().imageView;
        imageInfo.sampler = textureImage->getSampler();

        //Write the descriptor info to the buffer

        VkWriteDescriptorSet descriptorWrite{};

        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(logicalDevice, 1, &descriptorWrite, 0, nullptr);
    }

    //Injected device
    VkDevice logicalDevice;

    std::string name;
    std::shared_ptr<Image> textureImage;

    //MATERIAL AND MESH DESCRIPTOR SETS NEED TO BE CLEANED UP ON PROGRAM EXIT AS WELL --TODO
    VkDescriptorSet descriptorSet;
    VkDescriptorSetLayout descriptorSetLayout;
};

class Mesh {
public:
    virtual const std::vector<Vertex>& getVertices() const = 0;
    virtual const std::vector<uint32_t>& getIndices() const = 0;
    virtual glm::mat4 getModelMatrix() const = 0;
    virtual int getMeshIndex() const = 0;
    virtual void setMeshIndex(int index) = 0;
    virtual std::string getName() const = 0;
    virtual std::shared_ptr<Material> getMaterial() const = 0;


    virtual ~Mesh() = default;
};

class Plane : public Mesh {
public:
    Plane(std::shared_ptr<Material> material) : material(material), model(glm::mat4(1.0f)), meshIndex(-1) {}

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

private:
    glm::mat4 model;
    int meshIndex;
    std::shared_ptr<Material> material;

    const std::vector<Vertex> vertices = {
        {{-0.5f,  0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 0.5f,  0.0f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f,  0.0f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
    };

    const std::vector<uint32_t> indices = {
        0, 1, 2, 2, 3, 0
    };
};

class DynamicMesh : public Mesh {
public:
    DynamicMesh(std::string name, std::vector<Vertex> vertices, std::vector<uint32_t> indices, std::shared_ptr<Material> material)
        : name(std::move(name)), vertices(std::move(vertices)), indices(std::move(indices)), material(material), meshIndex(-1) {
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

private:
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    glm::mat4 model = glm::mat4(1.0f);
    int meshIndex;
    std::shared_ptr<Material> material;
};

class MeshManager {
public:
    MeshManager(VkDevice logicalDevice, std::shared_ptr<BufferManager> bufferManager);

    void loadModel(std::string filepath, std::string name, std::string materialName);

    void addMesh(std::shared_ptr<Mesh> mesh);

    std::shared_ptr<Material> createMaterial(std::string name,
        std::shared_ptr<Image> textureImage);

    std::shared_ptr<Mesh> createMesh(std::string meshName, std::string matName);

    void createStorageBuffer();

    void createMaterialDescriptorSetLayout();

    void createMeshDescriptorSetLayout();

    void createSSBODescriptorSet(VkDescriptorPool descriptorPool);

    void createMaterialDescriptorSets(VkDescriptorPool descriptorPool);

    void transform(std::string meshName, std::string transformType, uint32_t currentImage);

    // == GETTERS == 
    //Storage buffer set
    const std::unordered_map<std::string, std::shared_ptr<Mesh>>& getAllMeshes() const;
    const std::unordered_map<std::string, std::shared_ptr<Material>>& getAllMaterials() const { return materials; };
    const std::unordered_map<std::string, std::vector<std::shared_ptr<Mesh>>> getMeshesByMaterial() const { return meshesByMaterial; };
    
    //Sets 
    std::vector<VkDescriptorSet> getSSBODescriptorSets() { return meshDescriptorSets; };

    //Set layouts
    VkDescriptorSetLayout getMeshDescriptorSetLayout() { return meshDescriptorSetLayout; };
    VkDescriptorSetLayout getMaterialDescriptorSetLayout() { return materialDescriptorSetLayout; };

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
    VkDevice meshManager_logicalDevice;

    //Stores all meshes by name
    std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes;
    //Stores all materials by name
    std::unordered_map<std::string, std::shared_ptr<Material>> materials; 
    //Stores <name, meshIndex>
    std::unordered_map<std::string, int> meshIndices;
    //Stores all meshes by material, used for per material render loop
    std::unordered_map<std::string, std::vector<std::shared_ptr<Mesh>>> meshesByMaterial;

    int meshCount;

    //Buffer Management
    std::shared_ptr<BufferManager> meshManager_bufferManager;
    std::vector<void*> mappedStorageBufferPtrs;
    std::vector<glm::mat4> modelMatrices; 

    //Descriptor Management
    VkDescriptorSetLayout materialDescriptorSetLayout;
    VkDescriptorSetLayout meshDescriptorSetLayout;
    std::vector<VkDescriptorSet> meshDescriptorSets;

};

#endif