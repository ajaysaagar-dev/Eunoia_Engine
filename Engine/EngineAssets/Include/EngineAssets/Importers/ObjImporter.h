#pragma once
#include <string>
#include <EngineAssets/MeshImporter.h>

namespace EngineAssets {

class ObjImporter {
public:
    static ImportedModel Load(const std::string& filePath) {
        return MeshImporter::LoadOBJ(filePath);
    }
};

} // namespace EngineAssets
