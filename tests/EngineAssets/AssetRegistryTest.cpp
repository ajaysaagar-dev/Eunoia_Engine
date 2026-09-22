#include <iostream>
#include <cassert>
#include "EngineAssets/AssetRegistry.h"
#include "EngineAssets/AssetType.h"

int main() {
    std::cout << "[RUNNING] EngineAssets Tests...\n";

    auto& registry = AssetRegistry::Get();
    AssetMetadata meta;
    meta.id = AssetID::Generate();
    meta.type = AssetType::StaticMesh;
    meta.name = "TestMesh";
    meta.virtualPath = "/Game/Meshes/TestMesh";
    meta.filePath = "models/test.obj";

    bool regOk = registry.RegisterAsset(meta);
    assert(regOk);

    const AssetMetadata* found = registry.FindById(meta.id);
    assert(found != nullptr);
    assert(found->name == "TestMesh");

    const AssetMetadata* foundByPath = registry.FindByPath("/Game/Meshes/TestMesh");
    assert(foundByPath != nullptr);
    assert(foundByPath->id == meta.id);

    std::cout << "[PASSED] All EngineAssets Tests passed successfully!\n";
    return 0;
}
