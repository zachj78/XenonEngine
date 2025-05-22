#include "../include/MeshManager.h"
#include "../include/Buffer.h"
#include <tiny_obj_loader.h>

// == MESH MANAGER == 
MeshManager::MeshManager(VkDevice logicalDevice, std::shared_ptr<BufferManager> bufferManager)
    : meshManager_logicalDevice(logicalDevice), meshManager_bufferManager(bufferManager), meshCount(0) {
    createMeshDescriptorSetLayout();
}

// == INITIALIZATION == 

void MeshManager::addMesh(std::shared_ptr<Mesh> mesh) {
    std::cout << "Adding mesh: " << mesh->getName() << std::endl;

    modelMatrices.push_back(mesh->getModelMatrix());
    std::cout << "   matrix: [" << mesh->getModelMatrix()[0][0] << "]" << std::endl;

    // Store mesh by material data for rendering
    std::shared_ptr<Material> material = mesh->getMaterial();
    if (material) {
        const std::string& materialName = material->getName();
        std::cout << "   with material: [" << materialName << "]" << std::endl;

        if (materials.find(materialName) == materials.end()) {
            materials[materialName] = material;
        }

        meshesByMaterial[materialName].push_back(mesh);
    }

    const std::string& meshName = mesh->getName();
    meshes[meshName] = std::move(mesh);

    // Assign mesh index now that it's in the map
    int meshIndex = static_cast<int>(meshes.size() - 1);
    meshes[meshName]->setMeshIndex(meshIndex);
    meshIndices[meshName] = meshIndex;

    std::cout << "   set meshIndex: " << meshIndex << std::endl;

    meshCount++;
    std::cout << "   new mesh count: [" << meshCount << "]" << std::endl;
}

void MeshManager::loadModel(std::string filepath, std::string name, std::string materialName) {
    std::cout << "Loading model" << std::endl;

    std::shared_ptr<Material> material = getMaterial(materialName);

    std::string MODEL_PATH = filepath;
    std::vector<Vertex> vertices = {};
    std::vector<uint32_t> indices = {};

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
        throw std::runtime_error(warn + err);
    };

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = { 1.0f, 1.0f, 1.0f };

            vertices.push_back(vertex);
            indices.push_back(indices.size());
        }
    }

    auto mesh = std::make_shared<DynamicMesh>(name, vertices, indices, material);
    meshes[name] = std::move(mesh);

    std::cout << "Finished loading model" << std::endl;
};

std::shared_ptr<Material> MeshManager::createMaterial(std::string name,
    std::shared_ptr<Image> textureImage) {
    std::cout << "Creating material : [" << name << "] with image: [" << textureImage->getImage() << "]" << std::endl;

    std::shared_ptr<Material> mat = std::make_shared<Material>(name,
        textureImage,
        materialDescriptorSetLayout,
        meshManager_logicalDevice
    );

    return mat;
}

std::shared_ptr<Mesh> MeshManager::createMesh(std::string meshName, std::string matName) {
    std::cout << "Creating mesh : [" << meshName << "] with material name: [" << matName << "]" << std::endl;

    auto it = materials.find(matName);
    if (it == materials.end()) {
        std::cout << "Failed to create mesh -> could not find material name: [" << matName << "]" << std::endl;
    }

    std::shared_ptr<Material> mat = it->second;

    //PLUG MATERIAL IN -> DO NEXT
    std::shared_ptr<Mesh> plane = std::make_shared<Plane>(mat);
    return plane;
}

void MeshManager::createStorageBuffer() {
    std::cout << "Entered MeshManager::createStorageBuffer" << std::endl;

    VkDeviceSize storageBufSize = modelMatrices.size() * sizeof(glm::mat4);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        std::string bufName = "meshStorage" + std::to_string(i);

        meshManager_bufferManager->createBuffer(
            BufferType::STORAGE,
            bufName, 
            storageBufSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        std::shared_ptr<Buffer> meshStorageBuffer = meshManager_bufferManager->getBuffer(bufName);

        void* meshStorageBufferPtr;
        vkMapMemory(meshManager_logicalDevice, meshStorageBuffer->getMemory(), 0, storageBufSize, 0, &meshStorageBufferPtr);
        memcpy(meshStorageBufferPtr, modelMatrices.data(), storageBufSize);
        vkUnmapMemory(meshManager_logicalDevice, meshStorageBuffer->getMemory());
        
        mappedStorageBufferPtrs.push_back(meshStorageBufferPtr);
    }
};

void MeshManager::createMaterialDescriptorSetLayout() {
    std::cout << "Creating material descriptor set layout" << std::endl;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerLayoutBinding;

    VkResult result = vkCreateDescriptorSetLayout(meshManager_logicalDevice, &layoutInfo, nullptr, &materialDescriptorSetLayout);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout");
    }
    else {
        std::cout << "Created descriptor set layout successfully : [" << result << "]" << std::endl;
    };
}

void MeshManager::createMeshDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding storageBufferBinding{};
    storageBufferBinding.binding = 0;
    storageBufferBinding.descriptorCount = 1;
    storageBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    storageBufferBinding.pImmutableSamplers = nullptr;
    storageBufferBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &storageBufferBinding;

    if (vkCreateDescriptorSetLayout(meshManager_logicalDevice, &layoutInfo, nullptr, &meshDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout for MeshManager SSBO");
    }
}

void MeshManager::createSSBODescriptorSet(VkDescriptorPool descriptorPool) {
    meshDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        std::cout << "Creating SSBO Descriptor Set"  << i << std::endl;

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &meshDescriptorSetLayout;

        VkResult result = vkAllocateDescriptorSets(meshManager_logicalDevice, &allocInfo, &meshDescriptorSets[i]);
        if (result != VK_SUCCESS) {
            std::cerr << "vkAllocateDescriptorSets failed with error code: " << result << std::endl;
            throw std::runtime_error("Failed to allocate descriptor set for MeshManager SSBO");
        }

        std::string bufName = "meshStorage" + std::to_string(i);
        std::shared_ptr<Buffer> meshStorageBuffer = meshManager_bufferManager->getBuffer(bufName);

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = meshStorageBuffer->getHandle();
        bufferInfo.offset = 0;
        bufferInfo.range = modelMatrices.size() * sizeof(glm::mat4);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = meshDescriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(meshManager_logicalDevice, 1, &descriptorWrite, 0, nullptr);
    }
}

void MeshManager::createMaterialDescriptorSets(VkDescriptorPool descriptorPool) {
    std::cout << "Entering meshManager::createMaterialDescriptorSets" << std::endl;
    for (auto material : materials) {
        material.second->initDescriptors(descriptorPool, this->getMaterialDescriptorSetLayout());
    }
};

// == ACTUAL GRAPHICAL OUTPUT SHIT == 

void MeshManager::transform(std::string meshName, std::string transformType, uint32_t currentImage) {
    std::cout << "Calling mesh transform on mesh: [" << meshName << "]" << std::endl;

    int meshIndex = meshes[meshName]->getMeshIndex();

    // Get pointer to mapped memory for this frame
    void* rawPtr = mappedStorageBufferPtrs[currentImage];

    // Cast to MeshData*
    MeshData* meshDataArray = static_cast<MeshData*>(rawPtr);

    // == MOVE(translate) ==
    // -> "m{direction}_{axis}"
    // -> example "Move forward Z" = mf_z
    if (transformType == "mf_z") {
        meshDataArray[meshIndex].model = glm::translate(meshDataArray[meshIndex].model, glm::vec3(0.0f, 0.0f, -0.1f));
    }

    if (transformType == "mf_y") {
        meshDataArray[meshIndex].model = glm::translate(meshDataArray[meshIndex].model, glm::vec3(0.0f, 0.1f, 0.0f));
    }

    if (transformType == "mf_x") {
        meshDataArray[meshIndex].model = glm::translate(meshDataArray[meshIndex].model, glm::vec3(0.1f, 0.0f, 0.0f));
    }

    if (transformType == "mb_z") {
        meshDataArray[meshIndex].model = glm::translate(meshDataArray[meshIndex].model, glm::vec3(0.0f, 0.0f, 0.1f));
    }

    if (transformType == "mb_x") {
        meshDataArray[meshIndex].model = glm::translate(meshDataArray[meshIndex].model, glm::vec3(-0.1f, 0.0f, 0.0f));
    }

    if (transformType == "mb_y") {
        meshDataArray[meshIndex].model = glm::translate(meshDataArray[meshIndex].model, glm::vec3(0.0f, -0.1f, 0.0f));
    }

    // Write new model matrix to the mesh index
    meshDataArray[meshIndex].model = glm::rotate(meshDataArray[meshIndex].model, 45.0f, glm::vec3(0.0f, 1.0f, 0.0f));
}

// -- Getter functions -- 
const std::unordered_map<std::string, std::shared_ptr<Mesh>>& MeshManager::getAllMeshes() const {
    return meshes;
};