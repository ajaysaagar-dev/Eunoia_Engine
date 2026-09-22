#pragma once
// ============================================================================
// Editor::ViewportPanel — 3D viewport with gizmos and fly-camera input
// Was: gizmo + camera input inside EngineUI::Render() / RenderGizmo()
// ============================================================================

#include <EngineScene/Scene.h>
#include <EngineRenderer/Camera.h>

namespace Editor {

class ViewportPanel {
public:
    // Render the viewport overlay (gizmos, stats, toolbar).
    // viewportX/Y/W/H: screen-space rect of the actual 3D view.
    void Render(Scene& scene, OrbitCamera& camera,
                float viewportX, float viewportY,
                float viewportW, float viewportH);

    // Returns true if the gizmo is currently being dragged
    bool IsUsingGizmo() const { return m_gizmoActive; }

    // Current gizmo operation (Translate/Rotate/Scale)
    int GizmoOperation() const { return m_gizmoOperation; }
    int GizmoMode()      const { return m_gizmoMode; }

private:
    bool m_gizmoActive    = false;
    int  m_gizmoOperation = 0;   // ImGuizmo::TRANSLATE
    int  m_gizmoMode      = 0;   // ImGuizmo::WORLD
    bool m_useSnap        = false;
};

} // namespace Editor
