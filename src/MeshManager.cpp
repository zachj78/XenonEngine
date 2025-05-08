#include "../include/MeshManager.h"


// -- Getter functions -- 
const std::unordered_map<std::string, std::shared_ptr<Mesh>>& MeshManager::getAllMeshes() const {
    return meshes;
};
