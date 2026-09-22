#pragma once
#include <string>
#include <EngineAssets/MeshImporter.h>

namespace EngineAssets {

class FbxImporter {
public:
    static ImportedModel Load(const std::string& filePath) {
        return MeshImporter::LoadFBX(filePath);
    }
};

} // namespace EngineAssets
