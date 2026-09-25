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
    glm::vec3 baseColor{1.0f, 1.0f, 1.0f};
    float metallic = 0.0f;
    float roughness = 0.5f;
    float normalStrength = 1.0f;
    bool normalMapYFlip = false;
    int metallicChannel = 0;   // 0=R, 1=G, 2=B, 3=A
    int roughnessChannel = 1;  // 0=R, 1=G, 2=B, 3=A
    int aoChannel = 0;         // 0=R, 1=G, 2=B, 3=A
    int blendMode = 0;         // 0=Opaque, 1=Masked, 2=Translucent
    float opacity = 1.0f;
    float opacityMaskClipValue = 0.333f;
    glm::vec3 emissiveColor{0.0f, 0.0f, 0.0f};
    float emissiveIntensity = 0.0f;
    int materialDebugMode = 0;
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
