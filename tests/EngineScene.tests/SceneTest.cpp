#include <iostream>
#include <cassert>
#include "EngineScene/Scene.h"
#include "EngineScene/SceneSerializer.h"

int main() {
    std::cout << "[RUNNING] EngineScene Tests...\n";

    Scene scene;
    int parentId = scene.AddObject(PrimitiveType::Cube, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f));
    int childId = scene.AddObject(PrimitiveType::Sphere, glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(1.0f));

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

    std::cout << "[PASSED] All EngineScene Tests passed successfully!\n";
    return 0;
}
