#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "Geometry.h"

struct ImportedMesh {
    std::vector<MeshVertex> vertices;
    std::vector<uint32_t> indices;
    std::string name;
    bool valid = false;
};

struct ImportedModel {
    std::vector<ImportedMesh> meshes;
    bool valid = false;
    std::string errorMsg;
    
    // Merged mesh (all sub-meshes combined)
    PrimitiveMesh GetMergedMesh() const;
};

class MeshImporter {
public:
    static ImportedModel LoadOBJ(const std::string& filePath);
    static ImportedModel LoadGLTF(const std::string& filePath);
    static ImportedModel LoadFBX(const std::string& filePath);
    static ImportedModel Load(const std::string& filePath);
    static bool IsSupportedFormat(const std::string& ext);
};
