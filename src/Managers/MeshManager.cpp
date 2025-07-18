#include "../include/Managers/MeshManager.h"
#include "../include/Managers/Buffer.h"
#include "../include/Managers/ImageManager.h"

//3D Model Loader Imports
#include "../include/External/tiny_obj_loader.h"
#include "../include/External/tiny_gltf.h"

//GLTF loading helper function 
auto getFloats = [&](const tinygltf::Model& model, const tinygltf::Accessor& accessor) {
    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
    const float* data = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
    return data;
};

// == MESH MANAGER == 
MeshManager::MeshManager(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, std::shared_ptr<BufferManager> bufferManager)
    : meshManager_logicalDevice(logicalDevice), meshManager_physicalDevice(physicalDevice), meshManager_bufferManager(bufferManager), meshCount(0) {
}

// == MODEL LOADING FUNCTIONS == 
//For loading .obj models into a <Mesh> instance
void MeshManager::loadModel_obj(std::string filepath, std::string name, std::string materialName) 
{
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

            vertex.color = { 1.0f, 1.0f, 1.0f, 1.0f };

            vertices.push_back(vertex);
            indices.push_back(indices.size());
        }
    }

    std::vector<std::shared_ptr<Primitive>> meshPrimitives; 

    auto primitive = createPrimitive(vertices,
        indices,
        material,
        0, //Opaque alpha
        1, //back cull
        1, //Depth test and write on
        1,
        4 //assume triangle primitive
    );

    meshPrimitives.push_back(primitive);

    auto mesh = std::make_shared<Mesh>(name, meshPrimitives);

    meshes[name] = std::move(mesh);

    std::cout << "Finished loading model" << std::endl;
};

//For loading .gLTF models into a <Mesh> instance
void MeshManager::loadModel_gLTF(
    std::shared_ptr<BufferManager> bufferManager,
    std::shared_ptr<ImageManager> imageManager,
    VkDescriptorSetLayout descriptorSetLayout,
    VkCommandPool commandPool,
    std::string filepath, 
    std::string name
)
{
    std::cout << "\n Loading .gLTF model: " << filepath << std::endl;

    // Parse .glb
    std::string err, warn;
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;

    //ALL Vertex positions
    std::vector<uint32_t> indices; // THIS IS ONLY HERE FOR LOGGING PURPOSES, TO TRACK INDICES DRAWN FOR EACH PRIMITIVE
    size_t vertexOffset = 0;

    if (!loader.LoadBinaryFromFile(&model, &err, &warn, filepath)) {
        throw std::runtime_error(warn + err);
    };

    std::vector<tinygltf::Mesh> meshes = model.meshes;

    std::cout << "Loading model with " << meshes.size() << " meshes" << std::endl;

    for (size_t i = 0; i < meshes.size(); ++i) {
        std::cout << "\nLoading mesh" << i << "/" << meshes.size() << std::endl;

        std::vector<std::shared_ptr<Primitive>> meshPrimitives;

        // Extract Primitives
        for (size_t j = 0; j < meshes[i].primitives.size(); ++j) {
                std::cout << "\n -> Loading primitive " << j << " --- ";

                std::shared_ptr<Material> albedo;
                std::shared_ptr<Primitive> prim;

                const auto& primitive = meshes[i].primitives[j];

                // == GET PIPELINE KEY PER MESH PRIMITIVE ==

                //Pipeline key variables
                uint16_t alphaMode = 0;
                uint16_t cullMode = 0;
                uint16_t depthTest = 1;
                uint16_t depthWrite = 1;
                uint16_t topology = 4; 

                int materialIndex = primitive.material;
                const auto& material = model.materials[materialIndex];

                if (primitive.mode >= 0 && primitive.mode <= 6) {
                    topology = primitive.mode;
                }

                //blendModeID
                if (material.alphaMode == "OPAQUE") {
                    alphaMode = 0;
                } else if (material.alphaMode == "MASK") {
                    alphaMode = 1; 
                } else if (material.alphaMode == "BLEND") {
                    alphaMode = 2;
                    depthWrite = 0; 
                }

                //cullModeID
                cullMode = material.doubleSided ? 0 : 1; 

                // == EXTRACT MATERIALS == 

                // Albedo/Color
                if (material.values.find("baseColorTexture") != material.values.end()) {
                    std::cout << "Albedo material exists for primitive " << j << std::endl; 

                    int textureIndex = material.values.at("baseColorTexture").TextureIndex();
                    const auto& texture = model.textures[textureIndex];
                    auto& image = model.images[texture.source];

                    //[DEBUG]
                    std::cout << "Image name: " << image.name << std::endl;
                    std::cout << "URI: " << image.uri << std::endl;
                    std::cout << "Has image.image data? " << (!image.image.empty() ? "yes" : "no") << std::endl;
                    std::cout << "Width: " << image.width << ", Height: " << image.height << std::endl;
                    std::cout << "image.image.size(): " << image.image.size() << std::endl;

                    //check where is image is located at
                    if (!image.uri.empty()) {
                        std::cout << "Image @ " << image.uri << std::endl;
                    } else if (!image.image.empty()) {
                        std::cout << "Image is an embedded texture" << std::endl;

                        // Get image color format
                        std::string mime = image.mimeType;
                        std::string albedoName = name + "_albedo";

                        std::vector<unsigned char> imageSource;
                        int w = image.width;
                        int h = image.height;
                        int dummyChannels = 0;

                        bool isEncoded = stbi_info_from_memory(image.image.data(), image.image.size(), &w, &h, &dummyChannels);

                        if (isEncoded) {
                            std::cout << "Image is encoded (PNG/JPEG). Decoding..." << std::endl;
                            int channels;
                            unsigned char* decoded = stbi_load_from_memory(image.image.data(), image.image.size(), &w, &h, &channels, 4);
                            if (!decoded) throw std::runtime_error("Failed to decode embedded image");
                            imageSource.assign(decoded, decoded + w * h * 4);
                            stbi_image_free(decoded);
                        }
                        else {
                            std::cout << "Image is already raw pixel data. Using directly." << std::endl;
                            // Assume TinyGLTF already decoded it to RGBA8 (this is what it does by default)
                            imageSource = image.image;
                            if (imageSource.size() != w * h * 4) {
                                throw std::runtime_error("Image size does not match expected RGBA format");
                            }
                        }

                        imageManager->createTextureImage(albedoName, imageSource, w, h, commandPool);
                    
                        std::shared_ptr<Image> albedoImage = imageManager->getImage(albedoName);

                        createMaterial(albedoName, albedoImage);

                        albedo = getMaterial(albedoName);                    
                    }
                } else {
                    albedo = getMaterial("mat1");
                }

                // == EXTRACT VERTEX ATTRIBUTES 

                std::vector<Vertex> primitiveVertices;
                std::vector<uint32_t> primitiveIndices; 

                //Extract vertex positions
                if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
                    const auto& accessor = model.accessors[primitive.attributes.at("POSITION")];
                    const float* data = getFloats(model, accessor);

                    primitiveVertices.resize(accessor.count);

                    for (size_t i = 0; i < accessor.count; ++i) {

                        primitiveVertices[i].pos = glm::vec3(
                            data[i * 3 + 0],
                            data[i * 3 + 1],
                            data[i * 3 + 2]
                        );

                        //Default white color for now
                        primitiveVertices[i].color = { 1.0f, 1.0f, 1.0f, 1.0f };
                    }
                }

                //Extract Tex coords(UV)
                if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
                    const auto& accessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
                    const float* data = getFloats(model, accessor);
                    for (size_t i = 0; i < accessor.count && i < primitiveVertices.size(); ++i) {
                        primitiveVertices[i].texCoord = glm::vec2(
                            data[i * 2 + 0],
                            data[i * 2 + 1]
                        );
                    }
                }

                //Extract normals
                if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
                    const auto& accessor = model.accessors[primitive.attributes.at("NORMAL")];
                    const float* data = getFloats(model, accessor);

                    for (size_t i = 0; i < accessor.count; ++i) {
                        primitiveVertices[i].normal = glm::vec3(
                            data[i * 3 + 0],
                            data[i * 3 + 1],
                            data[i * 3 + 2]
                        );
                    }
                }

                //Extract tangents
                if (primitive.attributes.find("TANGENT") != primitive.attributes.end()) {
                    const auto& accessor = model.accessors[primitive.attributes.at("TANGENT")];
                    const float* data = getFloats(model, accessor);

                    for (size_t i = 0; i < accessor.count && i < primitiveVertices.size(); ++i) {
                        primitiveVertices[i].tangent = glm::vec4(
                            data[i * 4 + 0],
                            data[i * 4 + 1],
                            data[i * 4 + 2],
                            data[i * 4 + 3]
                        );
                    }
                }

                size_t initialIndexCount = indices.size();

                //Extract indices
                if (primitive.indices >= 0) {
                    const auto& indexAccessor = model.accessors[primitive.indices];
                    const auto& bufferView = model.bufferViews[indexAccessor.bufferView];
                    const auto& buffer = model.buffers[bufferView.buffer];

                    const unsigned char* data = buffer.data.data() + bufferView.byteOffset + indexAccessor.byteOffset;

                    for (size_t i = 0; i < indexAccessor.count; ++i) {
                        uint32_t index;

                        switch (indexAccessor.componentType) {
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                            index = static_cast<uint32_t>(*(reinterpret_cast<const uint8_t*>(data + i)));
                            break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                            index = static_cast<uint32_t>(*(reinterpret_cast<const uint16_t*>(data + i * 2)));
                            break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                            index = *(reinterpret_cast<const uint32_t*>(data + i * 4));
                            break;
                        default:
                            throw std::runtime_error("Unsupported index component type");
                        }

                        primitiveIndices.push_back(index);
                        indices.push_back(index + vertexOffset);
                    }

                    size_t added = indices.size() - initialIndexCount;
                    std::cout << "Parsed " << added << " indices for primitive." << std::endl;
                }

                std::cout << "Alpha mode (OPAQUE, MASK, BLENDED): " << alphaMode << "\n"
                    << "Cull Mode (none, back, front)" << cullMode << "\n"
                    << "Depth Test (t/f)" << depthTest << "\n"
                    << "Depth Write (t/f)" << depthWrite << "\n"
                    << "Topology (points, line, line loop, line strip, triangles, triangle strip, triangle fan)" << topology << std::endl;

                prim = createPrimitive(
                    primitiveVertices,
                    primitiveIndices,
                    albedo,
                    alphaMode, //Pipeline key variables
                    cullMode,
                    depthTest,
                    depthWrite,
                    topology
                );

                meshPrimitives.push_back(prim);
            }

        std::shared_ptr<Mesh> mesh = createMesh(name + std::to_string(i), meshPrimitives);
    }
}

// == MESH, PRIMITIVE AND MATERIAL INIT AND CREATION == 
void MeshManager::createMaterial(std::string name,
    std::shared_ptr<Image> textureImage) {
    std::cout << "Creating material : [" << name << "] with image: [" << textureImage->getImage() << "]" << std::endl;

    ShaderSet shaders{};
    shaders.frag = "main_frag";
    shaders.vert = "main_vert";

    std::shared_ptr<Material> mat = std::make_shared<Material>(name,
        textureImage,
        materialDescriptorSetLayout,
        std::make_shared<ShaderSet>(shaders),
        meshManager_logicalDevice
    );

    materials[name] = std::move(mat);
}

std::shared_ptr<Primitive> MeshManager::createPrimitive(
    std::vector<Vertex> vertices,
    std::vector<uint32_t> indices,
    std::shared_ptr<Material> material,
    uint16_t blendModeID,
    uint16_t cullModeID,
    uint16_t depthTestID,
    uint16_t depthWriteID,
    uint16_t topologyTypeID
) 
{
    int primitiveIndex = primitives.size();

    std::cout << "CREATING PRIMATIVE AT INDEX " << primitiveIndex << std::endl; 

    std::shared_ptr<Primitive> primitive = std::make_shared<Primitive>(
        vertices, 
        indices, 
        material, 
        blendModeID, 
        cullModeID, 
        depthTestID, 
        depthWriteID, 
        topologyTypeID,
        primitiveIndex
    );

    primitivesByPipelineKey[primitive->getPipelineKey()].push_back(primitive);
    
    primitives.push_back(primitive);
    
    return primitive;
}

std::shared_ptr<Mesh> MeshManager::createMesh(std::string meshName, std::vector<std::shared_ptr<Primitive>> primitives) {
    std::cout << "Creating mesh : [" << meshName << "]" << std::endl;
    
    std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(meshName, primitives);

    std::cout << "   matrix: [" << mesh->getModelMatrix()[0][0] << "]" << std::endl;

    modelMatrices.push_back(mesh->getModelMatrix());

    meshes[meshName] = mesh;

    int meshIndex = static_cast<int>(meshes.size() - 1);
    mesh->setMeshIndex(meshIndex);

    std::cout << "[DEBUG] modelMatrices.size() = " << modelMatrices.size() << std::endl;
    std::cout << "[DEBUG] Assigned mesh index: " << meshIndex << std::endl;

    std::cout << "Added mesh at index : " << meshIndex << std::endl;

    meshIndices[meshName] = meshIndex;

    std::cout << "   set meshIndex: " << meshIndex << std::endl;

    meshCount++;

    std::cout << "   new mesh count: [" << meshCount << "]" << std::endl;

    //[debug]
    std::cout << "added mesh to map :" << meshes[meshName]->getName() << std::endl;

    return mesh;
}

// == MESH DESCRRIPTOR SET UP == 
void MeshManager::createStorageBuffers() {
    std::cout << "==> Entered MeshManager::createStorageBuffers" << std::endl;

    //[DEBUG - 256 BYTES ALLOCATED(4 MAT4s)]
    VkDeviceSize storageBufSize = 4 * sizeof(glm::mat4);
    std::cout << "Creating SSBO for " << modelMatrices.size() << " matrices ("
        << storageBufSize << " bytes total)" << std::endl;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        std::string bufName = "meshStorage" + std::to_string(i);

        std::cout << "  [Frame " << i << "] Creating buffer: " << bufName << std::endl;

        meshManager_bufferManager->createBuffer(
            BufferType::STORAGE,
            bufName,
            storageBufSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        std::shared_ptr<Buffer> meshStorageBuffer = meshManager_bufferManager->getBuffer(bufName);

        if (!meshStorageBuffer) {
            std::cerr << "  [ERROR] Failed to retrieve buffer: " << bufName << std::endl;
            continue;
        }

        std::cout << "  [Frame " << i << "] Buffer handle: " << meshStorageBuffer->getHandle()
            << ", memory: " << meshStorageBuffer->getMemory() << std::endl;

        void* meshStorageBufferPtr = nullptr;
        VkResult result = vkMapMemory(meshManager_logicalDevice,
            meshStorageBuffer->getMemory(),
            0,
            storageBufSize,
            0,
            &meshStorageBufferPtr);
        if (result != VK_SUCCESS) {
            std::cerr << "  [ERROR] vkMapMemory failed with code: " << result << std::endl;
        }
        else {
            std::cout << "  [Frame " << i << "] Mapped memory at: " << meshStorageBufferPtr << std::endl;
            memcpy(meshStorageBufferPtr, modelMatrices.data(), storageBufSize);
            vkUnmapMemory(meshManager_logicalDevice, meshStorageBuffer->getMemory());
            std::cout << "  [Frame " << i << "] SSBO data copied and unmapped." << std::endl;
        }

        mappedStorageBufferPtrs.push_back(meshStorageBufferPtr);
    }

    std::cout << "<== Finished MeshManager::createStorageBuffers\n" << std::endl;
}

void MeshManager::createSSBODescriptors(VkDescriptorPool descriptorPool) {
    std::cout << "==> Entered MeshManager::createSSBODescriptors" << std::endl;

    bool layoutBuilt = false;
    meshDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        std::string bufName = "meshStorage" + std::to_string(i);
        std::shared_ptr<Buffer> meshStorageBuffer = meshManager_bufferManager->getBuffer(bufName);

        if (!meshStorageBuffer) {
            std::cerr << "  [ERROR] Buffer " << bufName << " not found!" << std::endl;
            continue;
        }

        std::cout << "  [Frame " << i << "] Creating descriptor set for buffer: " << bufName
            << " (handle: " << meshStorageBuffer->getHandle() << ")" << std::endl;

        VkBuffer bufferHandle = meshStorageBuffer->getHandle();

        //[DEBUG -> AGAIN, ALLOCATING 256 BYTES TO DESCRIPTOR RANGE]
        VkDeviceSize bufferRange = 4 * sizeof(glm::mat4);

        std::cout << "Buffer range size: " << bufferRange << std::endl;

        DescriptorBuilder builder = DescriptorBuilder::begin(meshManager_logicalDevice);

        builder.bindBuffer(
            0,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            bufferHandle,
            bufferRange
        );

        if (!layoutBuilt) {
            if (builder.buildLayout(meshDescriptorSetLayout, false)) {
                std::cout << "  Descriptor set layout built successfully." << std::endl;
                layoutBuilt = true;
            }
            else {
                std::cerr << "  [ERROR] Failed to build descriptor set layout!" << std::endl;
                return;
            }
        }

        if (!builder.buildSet(meshDescriptorSetLayout, meshDescriptorSets[i], descriptorPool, false)) {
            std::cerr << "  [ERROR] Failed to build descriptor set for frame " << i << std::endl;
        }
        else {
            std::cout << "  [Frame " << i << "] Descriptor set created: " << meshDescriptorSets[i] << std::endl;
        }
    }

    std::cout << "<== Finished MeshManager::createSSBODescriptors\n" << std::endl;
}

// == MATERIAL DESCRIPTOR SET UP == 
void MeshManager::createMaterialDescriptors(VkDescriptorPool descriptorPool, Capabilities &deviceCaps) {
    std::cout << " ===> Creating material descriptor set layout <=== " << std::endl;
    std::cout << " - with bindless? " << deviceCaps.supportsDescriptorIndexing;

    //Determine whether to use bindless or individual texture samplers based on device caps
    if (deviceCaps.supportsDescriptorIndexing) {
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
                VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT
            );

            if (!layoutBuilt) {
                builder.buildLayout(materialDescriptorSetLayout, true);
                layoutBuilt = true; 
            }

            builder.buildSet(materialDescriptorSetLayout, materialDescriptorSets[i], descriptorPool, true);
        };  
    } else {
        std::cout << "    without bindless indexing!" << std::endl;

        //ONE LAYOUT PER MATERIAL
        bool layoutBuilt = false;

        //each material type gets its own descriptor set
        for (const auto& [name, material] : materials) {
            std::shared_ptr<Image> albedo = material->getTextureImage();
            
            //DEBUG
            std::cout << "Creating descriptor set for material : " << material->getName() << std::endl;

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
                    builder.buildLayout(materialDescriptorSetLayout, false);
                    layoutBuilt = true;
                }

                builder.buildSet(materialDescriptorSetLayout, matSets[i], descriptorPool, false);
            }

            material->setDescriptorSets(matSets);
        }
    }
}

// == ACTUAL GRAPHICAL OUTPUT SHIT == 
void MeshManager::transform(std::string meshName, std::string transformType, uint32_t currentImage) {
    std::cout << "Calling mesh transform on mesh: [" << meshName << "]" << std::endl;

    int meshIndex = meshes[meshName]->getMeshIndex();

    std::cout << "mesh index from meshes map: " << meshIndex << std::endl;

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

    if (transformType == "scale_d") {
        glm::vec3 scaleFactor = { 0.5f, 0.5f, 0.5f };
        meshDataArray[meshIndex] = glm::scale(meshDataArray[meshIndex], scaleFactor);
    }

    // Write new model matrix to the mesh index
    meshDataArray[meshIndex] = glm::rotate(meshDataArray[meshIndex], 45.0f, glm::vec3(0.0f, 1.0f, 0.0f));
}

// == Getter functions == 
const std::unordered_map<std::string, std::shared_ptr<Mesh>>& MeshManager::getAllMeshes() const {
    return meshes;
};