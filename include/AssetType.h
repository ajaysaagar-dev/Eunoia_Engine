#pragma once
#include <string>
#include <algorithm>

enum class AssetType {
    Unknown,
    Texture,
    Material,
    Mesh,
    SkeletalMesh,
    Animation,
    Audio,
    Level,
    Scene = Level,
    Prefab,
    Shader,
    Font,
    Particle,
    Script,
    Data
};

inline const char* AssetTypeToString(AssetType type) {
    switch (type) {
        case AssetType::Texture:      return "Texture";
        case AssetType::Material:     return "Material";
        case AssetType::Mesh:         return "Mesh";
        case AssetType::SkeletalMesh: return "SkeletalMesh";
        case AssetType::Animation:    return "Animation";
        case AssetType::Audio:        return "Audio";
        case AssetType::Level:        return "Level";
        case AssetType::Prefab:       return "Prefab";
        case AssetType::Shader:       return "Shader";
        case AssetType::Font:         return "Font";
        case AssetType::Particle:     return "Particle";
        case AssetType::Script:       return "Script";
        case AssetType::Data:         return "Data";
        default:                      return "Unknown";
    }
}

inline AssetType StringToAssetType(const std::string& str) {
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    if (s == "texture")      return AssetType::Texture;
    if (s == "material")     return AssetType::Material;
    if (s == "mesh")         return AssetType::Mesh;
    if (s == "skeletalmesh") return AssetType::SkeletalMesh;
    if (s == "animation")    return AssetType::Animation;
    if (s == "audio")        return AssetType::Audio;
    if (s == "level" || s == "scene") return AssetType::Level;
    if (s == "prefab")       return AssetType::Prefab;
    if (s == "shader")       return AssetType::Shader;
    if (s == "font")         return AssetType::Font;
    if (s == "particle")     return AssetType::Particle;
    if (s == "script")       return AssetType::Script;
    if (s == "data")         return AssetType::Data;
    return AssetType::Unknown;
}

inline AssetType DetectAssetTypeFromExtension(const std::string& ext) {
    std::string e = ext;
    std::transform(e.begin(), e.end(), e.begin(), ::tolower);
    if (!e.empty() && e[0] == '.') e.erase(0, 1);

    if (e == "png" || e == "jpg" || e == "jpeg" || e == "bmp" || e == "tga" || e == "dds" || e == "hdr") {
        return AssetType::Texture;
    }
    if (e == "emat" || e == "mat") {
        return AssetType::Material;
    }
    if (e == "obj" || e == "fbx" || e == "gltf" || e == "glb") {
        return AssetType::Mesh;
    }
    if (e == "wav" || e == "mp3" || e == "ogg") {
        return AssetType::Audio;
    }
    if (e == "level" || e == "elevel" || e == "scene" || e == "escene" || e == "umap") {
        return AssetType::Level;
    }
    if (e == "prefab") {
        return AssetType::Prefab;
    }
    if (e == "hlsl" || e == "glsl" || e == "spv" || e == "vert" || e == "frag") {
        return AssetType::Shader;
    }
    if (e == "ttf" || e == "otf") {
        return AssetType::Font;
    }
    return AssetType::Unknown;
}

inline const char* GetAssetTypeIcon(AssetType type) {
    switch (type) {
        case AssetType::Texture:      return "🖼";
        case AssetType::Material:     return "🎨";
        case AssetType::Mesh:         return "🧊";
        case AssetType::SkeletalMesh: return "🦴";
        case AssetType::Animation:    return "🏃";
        case AssetType::Audio:        return "🔊";
        case AssetType::Level:        return "🌐";
        case AssetType::Prefab:       return "📦";
        case AssetType::Shader:       return "⚡";
        case AssetType::Font:         return "🔤";
        case AssetType::Particle:     return "✨";
        case AssetType::Script:       return "📜";
        case AssetType::Data:         return "📊";
        default:                      return "📄";
    }
}
