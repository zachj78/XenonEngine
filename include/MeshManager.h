#pragma once
#ifndef MESH_MANAGER_H
#define MESH_MANAGER_H

#include "Config.h"

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord; 

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }
};

class Mesh {
public:
    virtual const std::vector<Vertex>& getVertices() const = 0;
    virtual const std::vector<uint32_t>& getIndices() const = 0;
    virtual std::string getName() const = 0;
    virtual VkDeviceSize getSize() const = 0;
    virtual ~Mesh() = default;
};

class TriangleMesh : public Mesh {
    const std::vector<Vertex> vertices = {
       //Position        Color                Texture Coordinates
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, -0.5f, 0.0f},  {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f},   {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f},  {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
    };

    std::vector<uint32_t> indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
    };

    const std::vector<Vertex>& getVertices() const override {
        return vertices;
    };

    const std::vector<uint32_t>& getIndices() const override {
        return indices; 
    }

    std::string getName() const override {
        return "Triangle";
    };

    VkDeviceSize getSize() const override {
        VkDeviceSize verticesSize = sizeof(vertices[0]) * vertices.size();
        return verticesSize;
    };
};

class MeshManager {
public: 
    void addMesh(std::shared_ptr<Mesh> mesh) {
        meshes[mesh->getName()] = std::move(mesh);
    }

    const Mesh* getMesh(const std::string& name) const {
        auto it = meshes.find(name);
        if (it != meshes.end()) return it->second.get();
        return nullptr;
    };

    const std::unordered_map<std::string, std::shared_ptr<Mesh>>& getAllMeshes() const;

private: 
    std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes;
};

#endif