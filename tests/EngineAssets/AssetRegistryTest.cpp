#include <iostream>
#include <cassert>
#include "EngineAssets/AssetRegistry.h"
#include "EngineAssets/AssetType.h"

int main() {
    std::cout << "[RUNNING] EngineAssets Tests...\n";

    auto& registry = AssetRegistry::Get();
    AssetMetadata meta;
    meta.id = AssetID::CreateRandom();
    meta.type = AssetType::Mesh;
    meta.objectName = "TestMesh";
    meta.virtualPath = "/Game/Meshes/TestMesh";
    meta.sourcePath = "models/test.obj";

    bool regOk = registry.RegisterAsset(meta);
    assert(regOk);

    const AssetMetadata* found = registry.FindById(meta.id);
    assert(found != nullptr);
    assert(found->objectName == "TestMesh");

    const AssetMetadata* foundByPath = registry.FindByPath("/Game/Meshes/TestMesh");
    assert(foundByPath != nullptr);
    assert(foundByPath->id == meta.id);

    std::cout << "[PASSED] All EngineAssets Tests passed successfully!\n";
    return 0;
}
