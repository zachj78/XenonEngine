#include "../include/Managers/MeshManager.h"
#include "../include/Managers/Buffer.h"

//3D Model Loader Imports
#include "../include/External/tiny_obj_loader.h"
#include "../include/External/tiny_gltf.h"

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

//GLTF loading helper function 
auto getFloats = [&](const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
    const float* data = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
    return data;
};

//For loading .gLTF models into a <Mesh> instance
// NOTE: No material name is provided via params, .glb files provide all related model textures**
// NOTE: This is solely meant to load one mesh per model, anymore will break this function, to be improved upon
void MeshManager::loadModel_gLTF(std::string filepath, std::string name) {
    std::cout << "Loading .gLTF model: " << filepath << std::endl;

    // Parse .glb
    std::string err, warn; 
    tinygltf::Model model; 
    tinygltf::TinyGLTF loader;

    //ALL Vertex positions
    std::vector<Vertex> vertices; // pos, texCoord, color
    std::vector<uint32_t> indices;


    if (!loader.LoadBinaryFromFile(&model, &err, &warn, filepath)) {
        throw std::runtime_error(warn + err);
    };

    //  Ensure only 1 mesh is being loaded in
    if (model.meshes.size() > 1) {
        std::cerr << "More than 1 mesh in model.meshes, imported models must include 1 mesh only" << "\n" << 
        "Actual Mesh Count : " << model.meshes.size() << std::endl;
    } else {
        const tinygltf::Mesh& mesh = model.meshes[0];

        for (const auto& primitive : mesh.primitives) {

            //Extract vertex positions
            if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
                const auto& accessor = model.accessors[primitive.attributes.at("POSITION")];
                const float* data = getFloats(model, accessor);
                for (size_t i = 0; i < accessor.count; ++i) {
                    vertices[i].pos = glm::vec3(
                        data[i * 3 + 0],
                        data[i * 3 + 1],
                        data[i * 3 + 2]
                    );
                }
            }

            //Extract Tex coords(UV)
            if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
                const auto accessor = model.accessors[primitive.attributes.at("TEXCOORD_0")]; 
                const float* data = getFloats(model, accessor);
                for (size_t i = 0; i < accessor.count; ++i) {
                    vertices[i].texCoord = glm::vec2(
                        data[i * 2 + 0],
                        data[i * 2 + 1]
                    );
                }
            }
        }

        //Now parse indices

    }
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
    glm::mat4* meshDataArray = static_cast<glm::mat4*>(rawPtr);

    // == MOVE(translate) ==
    // -> "m{direction}_{axis}"
    // -> example "Move forward Z" = mf_z
    if (transformType == "mf_z") {
        meshDataArray[meshIndex] = glm::translate(meshDataArray[meshIndex], glm::vec3(0.0f, 0.0f, -0.1f));
    }

    if (transformType == "mf_y") {
        meshDataArray[meshIndex] = glm::translate(meshDataArray[meshIndex], glm::vec3(0.0f, 0.1f, 0.0f));
    }

    if (transformType == "mf_x") {
        meshDataArray[meshIndex] = glm::translate(meshDataArray[meshIndex], glm::vec3(0.1f, 0.0f, 0.0f));
    }

    if (transformType == "mb_z") {
        meshDataArray[meshIndex] = glm::translate(meshDataArray[meshIndex], glm::vec3(0.0f, 0.0f, 0.1f));
    }

    if (transformType == "mb_x") {
        meshDataArray[meshIndex] = glm::translate(meshDataArray[meshIndex], glm::vec3(-0.1f, 0.0f, 0.0f));
    }

    if (transformType == "mb_y") {
        meshDataArray[meshIndex] = glm::translate(meshDataArray[meshIndex], glm::vec3(0.0f, -0.1f, 0.0f));
    }

    // Write new model matrix to the mesh index
    meshDataArray[meshIndex] = glm::rotate(meshDataArray[meshIndex], 45.0f, glm::vec3(0.0f, 1.0f, 0.0f));
}

// == Getter functions == 
const std::unordered_map<std::string, std::shared_ptr<Mesh>>& MeshManager::getAllMeshes() const {
    return meshes;
};