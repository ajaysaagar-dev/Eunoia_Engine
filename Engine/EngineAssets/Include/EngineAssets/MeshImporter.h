#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <glm/glm.hpp>
#include <EngineAssets/Geometry.h>

struct ImportedMaterial {
    std::string name = "Default_Material";
    std::string baseColorTexture;
    std::string normalTexture;
    std::string roughnessTexture;
    std::string metallicTexture;
    std::string aoTexture;
    std::string emissionTexture;
    std::string opacityTexture;
    float metallic = 0.0f;
    float roughness = 0.5f;
    glm::vec3 emissiveColor{0.0f, 0.0f, 0.0f};
    float emissiveIntensity = 0.0f;
    bool hasMaterial = false; // false = importer found no material data; leave GameObject defaults alone
};

struct ImportedMesh {
    std::vector<MeshVertex> vertices;
    std::vector<uint32_t> indices;
    std::string name;
    bool valid = false;
    ImportedMaterial material;
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
    static void ClearCache();
    static void InvalidateCache(const std::string& filePath);
};
