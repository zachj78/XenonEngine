#include "../include/Managers/MeshManager.h"
#include "../include/Managers/Buffer.h"
#include "../include/External/tiny_obj_loader.h"

// == MESH MANAGER == 
//REFACTOR THIS TO INCLUDE AN ACTUAL INJECTED DEVICES CLASS 
// -> THEN GET THE PHYSICAL DEVICE CAPABILITIES AND SET THE deviceSupportBindless FLAG, THEN REFACTOR YOUR MATERIAL DESCRIPTORS TO INCLUDE A FALLBACK FOR IF BINDLESS IS NOT SUPPORTED
MeshManager::MeshManager(VkDevice logicalDevice, std::shared_ptr<BufferManager> bufferManager)
    : meshManager_logicalDevice(logicalDevice), meshManager_bufferManager(bufferManager), meshCount(0) {
}

// == INITIALIZATION == 

//This  function is for creating your own mesh, requires you make a mesh before and pass as param
void MeshManager::addMesh(std::shared_ptr<Mesh> mesh) {
    std::cout << "Adding mesh: " << mesh->getName() << std::endl;

    modelMatrices.push_back(mesh->getModelMatrix());
    std::cout << "   matrix: [" << mesh->getModelMatrix()[0][0] << "]" << std::endl;

    // Store mesh by material data for rendering
    std::shared_ptr<Material> material = mesh->getMaterial();
    if (material) {
        const std::string& materialName = material->getName();
        std::cout << "   with material: [" << materialName << "]" << std::endl;

        //Adds materials to map if not already
        if (materials.find(materialName) == materials.end()) {
            materials[materialName] = material;
        }
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

//For loading .obj models into a <Mesh> instance
void MeshManager::loadModel_obj(std::string filepath, std::string name, std::string materialName) {
    std::cout << "Loading .obj model" << filepath << std::endl;

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

//For loading .gLTF models into a <Mesh> instance
void MeshManager::loadModel_gLTF(std::string filepath, std::string name, std::string materialName) {
    std::cout << "Loading .gLTF model: " << filepath << std::endl;
}

std::shared_ptr<Material> MeshManager::createMaterial(std::string name,
    std::shared_ptr<Image> textureImage) {
    std::cout << "Creating material : [" << name << "] with image: [" << textureImage->getImage() << "]" << std::endl;

    std::shared_ptr<Material> mat = std::make_shared<Material>(name,
        textureImage,
        materialDescriptorSetLayout,
        meshManager_logicalDevice
    );

    materials[name] = std::move(mat);

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

// == MESH DESCRRIPTOR SET UP == 
void MeshManager::createStorageBuffers() {
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
void MeshManager::createSSBODescriptors(VkDescriptorPool descriptorPool) {

    bool layoutBuilt = false;
    
    meshDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        DescriptorBuilder builder = DescriptorBuilder::begin(meshManager_logicalDevice);

        std::cout << "Creating SSBO Descriptor Set with builder" << i << std::endl;

        std::string bufName = "meshStorage" + std::to_string(i);
        std::shared_ptr<Buffer> meshStorageBuffer = meshManager_bufferManager->getBuffer(bufName);

        VkBuffer bufferHandle = meshStorageBuffer->getHandle();
        VkDeviceSize bufferRange = modelMatrices.size() * sizeof(glm::mat4);

        builder.bindBuffer(
            0,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            bufferHandle,
            bufferRange
        );

        if (layoutBuilt == false) {
            builder.buildLayout(meshDescriptorSetLayout);
            layoutBuilt = true; 
        };

        builder.buildSet(meshDescriptorSetLayout, meshDescriptorSets[i], descriptorPool);
    };
}

// == MATERIAL DESCRIPTOR SET UP == 
void MeshManager::createMaterialDescriptors(VkDescriptorPool descriptorPool, Capabilities deviceCaps) {
    std::cout << "Creating material descriptor set layout" << std::endl;
    
    //Determine whether to use bindless or individual texture samplers based on device caps
    if (deviceCaps.supportsBindless) {
        std::cout << "    with bindless indexing!" << std::endl;

        bool layoutBuilt = false;
        std::vector<VkImageView> views;
        std::vector<VkSampler> samplers;

        views.reserve(meshCount * textureTypes);
        samplers.reserve(meshCount * textureTypes);

        for (const auto& [name, material] : materials) {
            std::shared_ptr<Image> albedo = material->getTextureImage();
            views.push_back(albedo->getImageDetails().imageView);
            samplers.push_back(albedo->getSampler());
        }

        materialDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            DescriptorBuilder builder = DescriptorBuilder::begin(meshManager_logicalDevice);

            builder.bindImageArray(
                0,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                views,
                samplers,
                VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
            );

            if (!layoutBuilt) {
                builder.buildLayout(materialDescriptorSetLayout);
                layoutBuilt = true; 
            }

            builder.buildSet(materialDescriptorSetLayout, materialDescriptorSets[i], descriptorPool);
        };  
    } else {
        std::cout << "    without bindless indexing!" << std::endl;

        //ONE LAYOUT PER MATERIAL
        bool layoutBuilt = false;

        //each material type gets its own descriptor set
        for (const auto& [name, material] : materials) {
            std::shared_ptr<Image> albedo = material->getTextureImage();

            std::vector<VkDescriptorSet> matSets;
            matSets.resize(MAX_FRAMES_IN_FLIGHT);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                DescriptorBuilder builder = DescriptorBuilder::begin(meshManager_logicalDevice);

                builder.bindImage(
                    0,
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    VK_SHADER_STAGE_FRAGMENT_BIT,
                    albedo->getImageDetails().imageView,
                    albedo->getSampler()
                );

                if (!layoutBuilt) {
                    builder.buildLayout(materialDescriptorSetLayout);
                    layoutBuilt = true;
                }

                builder.buildSet(materialDescriptorSetLayout, matSets[i], descriptorPool);
            }

            material->setDescriptorSets(matSets);
        }
    }
}

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

// == Getter functions == 
const std::unordered_map<std::string, std::shared_ptr<Mesh>>& MeshManager::getAllMeshes() const {
    return meshes;
};