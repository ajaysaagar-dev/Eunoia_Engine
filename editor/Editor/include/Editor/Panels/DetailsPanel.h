#pragma once
// ============================================================================
// Editor::DetailsPanel — transform, material, and light properties (Right sidebar)
// Was: inline inside EngineUI::RenderDetails() in src/EngineUI.cpp
// ============================================================================

#include <EngineScene/Scene.h>
#include <EngineRenderer/Camera.h>

namespace Editor {

class DetailsPanel {
public:
    // Renders the Details panel window.
    void Render(Scene& scene, OrbitCamera& camera);

private:
    // Internal helpers (matching the split-out DrawTransformPill approach)
    void DrawTransformPill(const char* label, float* values, float resetValue, float columnWidth = 100.0f);
    void DrawLightComponent(Scene& scene, GameObject& obj);
    void DrawMaterialSection(Scene& scene, GameObject& obj);
};

} // namespace Editor
