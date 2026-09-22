#include <iostream>
#include <cassert>
#include "EngineScene/Scene.h"
#include "EngineScene/SceneSerializer.h"
#include "EngineScene/BehaviourRegistry.h"

void AddEngineLog(const std::string&, const std::string&, int) {}

int main() {
    std::cout << "[RUNNING] EngineScene Tests...\n";

    Scene scene;
    int parentId = scene.AddObject(PrimitiveType::Cube, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f)).id;
    int childId = scene.AddObject(PrimitiveType::Sphere, glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(1.0f)).id;

    scene.SetParent(childId, parentId);
    auto* parent = scene.FindObject(parentId);
    auto* child = scene.FindObject(childId);

    assert(parent != nullptr);
    assert(child != nullptr);
    assert(child->parentId == parentId);
    assert(!parent->childIds.empty());
    assert(parent->childIds[0] == childId);

    // World matrix test with parent scale
    parent->scale = glm::vec3(2.0f);
    glm::mat4 childWorld = scene.GetWorldMatrix(*child);
    // Child local pos (0, 2, 0) scaled by parent 2 -> world pos (0, 4, 0)
    assert(std::abs(childWorld[3][1] - 4.0f) < 1e-4f);

    // Test: Behaviour Object Reference & Selection Persistence across Play Mode
    int lightId = scene.AddObject(PrimitiveType::PointLight, glm::vec3(5.0f, 5.0f, 5.0f)).id;
    auto flicker = std::make_shared<LightFlickerBehaviour>();
    parent->AddBehaviour(flicker);
    scene.selectedId = parentId;

    // Assign reference to target light
    auto& props = flicker->GetProperties();
    for (auto& p : props) {
        if (p.name == "Target Light") {
            p.targetId = lightId;
            break;
        }
    }
    flicker->ResolveReferences(scene);
    assert(flicker->targetLight != nullptr);

    // Enter Play Mode
    scene.StartPlayMode();
    assert(scene.isPlayMode);

    // Exit Play Mode
    scene.StopPlayMode();
    assert(!scene.isPlayMode);

    // Verify selection is preserved (not deselected)
    assert(scene.selectedId == parentId);

    // Verify assigned object reference is preserved (does not disappear)
    auto* restoredParent = scene.FindObject(parentId);
    assert(restoredParent != nullptr);
    assert(!restoredParent->behaviours.empty());
    auto* restoredFlicker = dynamic_cast<LightFlickerBehaviour*>(restoredParent->behaviours[0].get());
    assert(restoredFlicker != nullptr);
    assert(restoredFlicker->targetLight != nullptr);
    bool foundTarget = false;
    for (auto& p : restoredFlicker->GetProperties()) {
        if (p.name == "Target Light") {
            assert(p.targetId == lightId);
            foundTarget = true;
            break;
        }
    }
    assert(foundTarget);

    std::cout << "[PASSED] All EngineScene Tests passed successfully!\n";
    return 0;
}
