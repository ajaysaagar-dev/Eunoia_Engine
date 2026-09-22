#pragma once
#include <string>
#include <EngineAssets/MeshImporter.h>

namespace EngineAssets {

class GltfImporter {
public:
    static ImportedModel Load(const std::string& filePath) {
        return MeshImporter::LoadGLTF(filePath);
    }
};

} // namespace EngineAssets
