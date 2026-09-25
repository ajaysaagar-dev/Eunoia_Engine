#pragma once
// ============================================================================
// Eunoia Engine — Editor Light & Camera Gizmo System (dev.md)
// Provides clean, depth-aware 3D debug visualizations for Lights and Cameras.
// Only visible in Editor Mode; completely omitted during runtime / Play Mode.
// ============================================================================

#include <EngineScene/Scene.h>
#include <EngineScene/GameObject.h>
#include <EngineAssets/Geometry.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>

namespace Eunoia {

using RenderBatch = Scene::RenderBatch;

class EditorGizmoSystem {
public:
    static EditorGizmoSystem& Get() {
        static EditorGizmoSystem s_instance;
        return s_instance;
    }

    // Editor Settings (dev.md Section 13)
    bool showLightGizmos       = true;
    bool showCameraGizmos      = true;
    bool showLightRanges       = true;
    bool showLightDirections   = true;
    bool showCameraFrustums    = true;
    bool showClipPlanes        = true;

    // Debug rendering primitives (dev.md Section 10)
    void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& color, float thickness = 1.0f);
    void DrawRay(const glm::vec3& origin, const glm::vec3& dir, float length, const glm::vec3& color, float thickness = 1.0f);
    void DrawArrow(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, float headSize = 0.35f, float thickness = 1.0f);
    void DrawCircle(const glm::vec3& center, const glm::vec3& normal, float radius, const glm::vec3& color, int segments = 36, float thickness = 1.0f);
    void DrawSphere(const glm::vec3& center, float radius, const glm::vec3& color, int segments = 36, float thickness = 1.0f);
    void DrawCone(const glm::vec3& apex, const glm::vec3& dir, float length, float angleDegrees, const glm::vec3& color, int segments = 24, float thickness = 1.0f);
    void DrawFrustum(const glm::vec3& pos, const glm::vec3& forward, const glm::vec3& up,
                     float fovYDegrees, float aspect, float nearClip, float farClip,
                     const glm::vec3& color, bool showClipPlanes = true, float thickness = 1.0f);
    void DrawBox(const glm::vec3& center, const glm::vec3& extents, const glm::mat4& rotation, const glm::vec3& color, float thickness = 1.0f);
    void DrawRectangle(const glm::vec3& center, const glm::vec3& uAxis, const glm::vec3& vAxis, float halfWidth, float halfHeight, const glm::vec3& color, float thickness = 1.0f);

    // Component-level Gizmo renderers (dev.md Section 2 - 7)
    void DrawPointLight(const GameObject& obj, const glm::mat4& worldMat);
    void DrawSpotLight(const GameObject& obj, const glm::mat4& worldMat);
    void DrawDirectionalLight(const GameObject& obj, const glm::mat4& worldMat);
    void DrawAreaLight(const GameObject& obj, const glm::mat4& worldMat);
    void DrawCameraFrustum(const GameObject& obj, const glm::mat4& worldMat, bool isActiveCamera);

    // Main render call invoked during geometry update
    void RenderGizmos(const Scene& scene, const glm::vec3& cameraPos,
                      std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices,
                      std::vector<RenderBatch>& outBatches);

private:
    EditorGizmoSystem() = default;

    std::vector<Vertex>* m_currentVertices = nullptr;
    std::vector<uint32_t>* m_currentIndices = nullptr;
    glm::vec3 m_cameraPos{0.0f};
};

} // namespace Eunoia
