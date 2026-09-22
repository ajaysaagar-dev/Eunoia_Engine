#pragma once
#include "AssetID.h"
#include "AssetType.h"
#include "AssetPath.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

struct AssetMetadata {
    AssetID id;
    AssetType type = AssetType::Unknown;

    std::string virtualPath;  // e.g. "/Game/Textures/Wood/T_Wood_Albedo"
    std::string objectName;   // e.g. "T_Wood_Albedo"

    std::string sourcePath;   // e.g. "Materials/textures/wood_albedo.png" (relative to project root)
    std::string cookedPath;   // e.g. "Cache/Textures/8F32A72C91DE.tex"

    std::vector<AssetID> dependencies; // Assets this asset requires
    std::vector<AssetID> references;   // Assets that require this asset

    uint64_t sourceTimestamp = 0;
    uint64_t sourceSize = 0;

    bool imported = true;
    bool cooked = false;
    bool isMissing = false;

    std::unordered_map<std::string, std::string> tags;

    bool IsValid() const {
        return id.IsValid() && !virtualPath.empty();
    }
};
