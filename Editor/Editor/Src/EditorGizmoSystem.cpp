#include "Editor/EditorGizmoSystem.h"
#include <algorithm>
#include <cmath>

namespace Eunoia {

void EditorGizmoSystem::DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& color, float thickness) {
    if (!m_currentVertices || !m_currentIndices) return;
    GeometryBuilder::AppendCameraFacingLine(*m_currentVertices, *m_currentIndices, p0, p1, m_cameraPos, thickness, color);
}

void EditorGizmoSystem::DrawRay(const glm::vec3& origin, const glm::vec3& dir, float length, const glm::vec3& color, float thickness) {
    float len = glm::length(dir);
    if (len < 1e-5f) return;
    glm::vec3 normDir = dir / len;
    DrawLine(origin, origin + normDir * length, color, thickness);
}

void EditorGizmoSystem::DrawArrow(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, float headSize, float thickness) {
    glm::vec3 dir = end - start;
    float len = glm::length(dir);
    if (len < 1e-5f) return;
    glm::vec3 normDir = dir / len;

    DrawLine(start, end, color, thickness);

    // Compute two orthonormal vectors perpendicular to direction
    glm::vec3 ref = std::abs(normDir.y) < 0.99f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 side = glm::normalize(glm::cross(normDir, ref));
    glm::vec3 up = glm::normalize(glm::cross(side, normDir));

    float actualHead = std::min(headSize, len * 0.4f);
    float headWidth = actualHead * 0.35f;
    glm::vec3 base = end - normDir * actualHead;

    // 4 fins for 3D arrow head
    DrawLine(end, base + side * headWidth, color, thickness * 1.1f);
    DrawLine(end, base - side * headWidth, color, thickness * 1.1f);
    DrawLine(end, base + up * headWidth, color, thickness * 1.1f);
    DrawLine(end, base - up * headWidth, color, thickness * 1.1f);

    // Connect base perimeter
    DrawLine(base + side * headWidth, base + up * headWidth, color, thickness * 0.85f);
    DrawLine(base + up * headWidth, base - side * headWidth, color, thickness * 0.85f);
    DrawLine(base - side * headWidth, base - up * headWidth, color, thickness * 0.85f);
    DrawLine(base - up * headWidth, base + side * headWidth, color, thickness * 0.85f);
}

void EditorGizmoSystem::DrawCircle(const glm::vec3& center, const glm::vec3& normal, float radius, const glm::vec3& color, int segments, float thickness) {
    if (radius <= 1e-4f || segments < 3) return;
    glm::vec3 n = glm::normalize(normal);
    glm::vec3 ref = std::abs(n.y) < 0.99f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 u = glm::normalize(glm::cross(n, ref));
    glm::vec3 v = glm::normalize(glm::cross(n, u));

    const float twoPi = 6.28318530717958647692f;
    float step = twoPi / (float)segments;
    glm::vec3 prev = center + u * radius;

    for (int i = 1; i <= segments; ++i) {
        float angle = (float)i * step;
        glm::vec3 curr = center + (u * std::cos(angle) + v * std::sin(angle)) * radius;
        DrawLine(prev, curr, color, thickness);
        prev = curr;
    }
}

void EditorGizmoSystem::DrawSphere(const glm::vec3& center, float radius, const glm::vec3& color, int segments, float thickness) {
    if (radius <= 1e-4f) return;

    // 3 primary orthogonal great circles
    DrawCircle(center, glm::vec3(0.0f, 1.0f, 0.0f), radius, color, segments, thickness); // XZ (Equator)
    DrawCircle(center, glm::vec3(0.0f, 0.0f, 1.0f), radius, color, segments, thickness); // XY
    DrawCircle(center, glm::vec3(1.0f, 0.0f, 0.0f), radius, color, segments, thickness); // YZ

    // 2 latitude rings at +/- 45 degrees
    float latAngle = 0.785398f; // 45 deg in rad
    float latRadius = radius * std::cos(latAngle);
    float latY = radius * std::sin(latAngle);
    DrawCircle(center + glm::vec3(0.0f, latY, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), latRadius, color * 0.75f, segments, thickness * 0.8f);
    DrawCircle(center - glm::vec3(0.0f, latY, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), latRadius, color * 0.75f, segments, thickness * 0.8f);
}

void EditorGizmoSystem::DrawCone(const glm::vec3& apex, const glm::vec3& dir, float length, float angleDegrees, const glm::vec3& color, int segments, float thickness) {
    if (length <= 1e-4f || angleDegrees <= 1e-4f) return;
    glm::vec3 nDir = glm::normalize(dir);
    float halfAngleRad = glm::radians(angleDegrees);
    float baseRadius = length * std::tan(halfAngleRad);
    glm::vec3 baseCenter = apex + nDir * length;

    // End boundary circle
    DrawCircle(baseCenter, nDir, baseRadius, color, segments, thickness);

    // 8 connecting rays from apex to base circle
    glm::vec3 ref = std::abs(nDir.y) < 0.99f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 u = glm::normalize(glm::cross(nDir, ref));
    glm::vec3 v = glm::normalize(glm::cross(nDir, u));

    const float twoPi = 6.28318530717958647692f;
    for (int i = 0; i < 8; ++i) {
        float angle = (float)i * (twoPi / 8.0f);
        glm::vec3 rimPoint = baseCenter + (u * std::cos(angle) + v * std::sin(angle)) * baseRadius;
        DrawLine(apex, rimPoint, color, thickness);
    }

    // Center ray
    DrawLine(apex, baseCenter, color * 0.8f, thickness * 0.75f);
}

void EditorGizmoSystem::DrawFrustum(const glm::vec3& pos, const glm::vec3& forward, const glm::vec3& up,
                                   float fovYDegrees, float aspect, float nearClip, float farClip,
                                   const glm::vec3& color, bool showClipPlanes, float thickness)
{
    glm::vec3 fwd = glm::normalize(forward);
    glm::vec3 r = glm::normalize(glm::cross(fwd, up));
    glm::vec3 u = glm::normalize(glm::cross(r, fwd));

    float fovYRad = glm::radians(fovYDegrees);
    float tanHalfFov = std::tan(fovYRad * 0.5f);

    float dNear = std::max(0.1f, nearClip);
    float dFar = std::clamp(farClip, dNear + 0.5f, 35.0f);

    float hNear = 2.0f * dNear * tanHalfFov;
    float wNear = hNear * aspect;
    float hFar = 2.0f * dFar * tanHalfFov;
    float wFar = hFar * aspect;

    glm::vec3 nearCenter = pos + fwd * dNear;
    glm::vec3 farCenter = pos + fwd * dFar;

    // Near corners
    glm::vec3 nTL = nearCenter + u * (hNear * 0.5f) - r * (wNear * 0.5f);
    glm::vec3 nTR = nearCenter + u * (hNear * 0.5f) + r * (wNear * 0.5f);
    glm::vec3 nBR = nearCenter - u * (hNear * 0.5f) + r * (wNear * 0.5f);
    glm::vec3 nBL = nearCenter - u * (hNear * 0.5f) - r * (wNear * 0.5f);

    // Far corners
    glm::vec3 fTL = farCenter + u * (hFar * 0.5f) - r * (wFar * 0.5f);
    glm::vec3 fTR = farCenter + u * (hFar * 0.5f) + r * (wFar * 0.5f);
    glm::vec3 fBR = farCenter - u * (hFar * 0.5f) + r * (wFar * 0.5f);
    glm::vec3 fBL = farCenter - u * (hFar * 0.5f) - r * (wFar * 0.5f);

    // 1. Near plane rectangle
    if (showClipPlanes) {
        DrawLine(nTL, nTR, color * 0.85f, thickness);
        DrawLine(nTR, nBR, color * 0.85f, thickness);
        DrawLine(nBR, nBL, color * 0.85f, thickness);
        DrawLine(nBL, nTL, color * 0.85f, thickness);
    }

    // 2. Far plane rectangle
    DrawLine(fTL, fTR, color, thickness);
    DrawLine(fTR, fBR, color, thickness);
    DrawLine(fBR, fBL, color, thickness);
    DrawLine(fBL, fTL, color, thickness);

    // 3. Four connecting side edges from near to far
    DrawLine(nTL, fTL, color, thickness);
    DrawLine(nTR, fTR, color, thickness);
    DrawLine(nBR, fBR, color, thickness);
    DrawLine(nBL, fBL, color, thickness);

    // Origin to near corners
    DrawLine(pos, nTL, color * 0.6f, thickness * 0.75f);
    DrawLine(pos, nTR, color * 0.6f, thickness * 0.75f);
    DrawLine(pos, nBR, color * 0.6f, thickness * 0.75f);
    DrawLine(pos, nBL, color * 0.6f, thickness * 0.75f);

    // 4. Orientation "hat" / top arrow on the far top edge to indicate roll / UP direction
    glm::vec3 topMid = (fTL + fTR) * 0.5f;
    glm::vec3 topApex = topMid + u * (hFar * 0.25f);
    DrawLine(topMid - r * (wFar * 0.15f), topApex, color, thickness * 1.25f);
    DrawLine(topMid + r * (wFar * 0.15f), topApex, color, thickness * 1.25f);
}

void EditorGizmoSystem::DrawRectangle(const glm::vec3& center, const glm::vec3& uAxis, const glm::vec3& vAxis, float halfWidth, float halfHeight, const glm::vec3& color, float thickness) {
    glm::vec3 p0 = center - uAxis * halfWidth - vAxis * halfHeight;
    glm::vec3 p1 = center + uAxis * halfWidth - vAxis * halfHeight;
    glm::vec3 p2 = center + uAxis * halfWidth + vAxis * halfHeight;
    glm::vec3 p3 = center - uAxis * halfWidth + vAxis * halfHeight;

    DrawLine(p0, p1, color, thickness);
    DrawLine(p1, p2, color, thickness);
    DrawLine(p2, p3, color, thickness);
    DrawLine(p3, p0, color, thickness);

    // Subtle crosshair inside rectangle
    DrawLine(center - uAxis * (halfWidth * 0.35f), center + uAxis * (halfWidth * 0.35f), color * 0.65f, thickness * 0.75f);
    DrawLine(center - vAxis * (halfHeight * 0.35f), center + vAxis * (halfHeight * 0.35f), color * 0.65f, thickness * 0.75f);
}

void EditorGizmoSystem::DrawBox(const glm::vec3& center, const glm::vec3& extents, const glm::mat4& rotation, const glm::vec3& color, float thickness) {
    glm::vec3 h = extents * 0.5f;
    glm::vec3 c[8] = {
        {-h.x, -h.y, -h.z}, { h.x, -h.y, -h.z}, { h.x, -h.y,  h.z}, {-h.x, -h.y,  h.z},
        {-h.x,  h.y, -h.z}, { h.x,  h.y, -h.z}, { h.x,  h.y,  h.z}, {-h.x,  h.y,  h.z}
    };
    for (int i = 0; i < 8; ++i) {
        c[i] = center + glm::vec3(rotation * glm::vec4(c[i], 0.0f));
    }
    DrawLine(c[0], c[1], color, thickness); DrawLine(c[1], c[2], color, thickness);
    DrawLine(c[2], c[3], color, thickness); DrawLine(c[3], c[0], color, thickness);
    DrawLine(c[4], c[5], color, thickness); DrawLine(c[5], c[6], color, thickness);
    DrawLine(c[6], c[7], color, thickness); DrawLine(c[7], c[4], color, thickness);
    DrawLine(c[0], c[4], color, thickness); DrawLine(c[1], c[5], color, thickness);
    DrawLine(c[2], c[6], color, thickness); DrawLine(c[3], c[7], color, thickness);
}

void EditorGizmoSystem::DrawPointLight(const GameObject& obj, const glm::mat4& worldMat) {
    glm::vec3 pos = glm::vec3(worldMat[3]);

    // Position marker (3-axis star/crosshair)
    float marker = 0.35f;
    glm::vec3 markerCol(1.0f, 0.88f, 0.35f);
    DrawLine(pos - glm::vec3(marker, 0, 0), pos + glm::vec3(marker, 0, 0), markerCol, 1.25f);
    DrawLine(pos - glm::vec3(0, marker, 0), pos + glm::vec3(0, marker, 0), markerCol, 1.25f);
    DrawLine(pos - glm::vec3(0, 0, marker), pos + glm::vec3(0, 0, marker), markerCol, 1.25f);

    // Spherical influence range visualization
    if (showLightRanges && obj.light.range > 0.01f) {
        glm::vec3 sphereColor = glm::mix(glm::vec3(1.0f, 0.82f, 0.22f), obj.light.color, 0.35f);
        DrawSphere(pos, obj.light.range, sphereColor, 48, 1.25f);

        // Optional 50% attenuation reference circle
        if (obj.light.attenuation > 0.0f) {
            DrawCircle(pos, glm::vec3(0.0f, 1.0f, 0.0f), obj.light.range * 0.5f, sphereColor * 0.6f, 36, 0.8f);
        }
    }
}

void EditorGizmoSystem::DrawSpotLight(const GameObject& obj, const glm::mat4& worldMat) {
    glm::vec3 apex = glm::vec3(worldMat[3]);

    glm::vec3 localDir = obj.light.direction;
    if (glm::length(localDir) < 1e-4f) localDir = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 fwd = glm::normalize(glm::vec3(worldMat * glm::vec4(localDir, 0.0f)));

    glm::vec3 coneColor(1.0f, 0.78f, 0.18f);

    // Outer cone
    float outerAngle = std::clamp(obj.light.outerConeAngle, 0.5f, 89.0f);
    DrawCone(apex, fwd, obj.light.range, outerAngle, coneColor, 36, 1.25f);

    // Inner cone
    float innerAngle = std::clamp(obj.light.innerConeAngle, 0.1f, outerAngle - 0.2f);
    if (innerAngle > 0.5f) {
        glm::vec3 innerColor(1.0f, 0.95f, 0.55f);
        DrawCone(apex, fwd, obj.light.range, innerAngle, innerColor, 24, 0.95f);
    }

    // Forward direction arrow
    if (showLightDirections) {
        float arrowLen = std::min(obj.light.range, 3.0f);
        DrawArrow(apex, apex + fwd * arrowLen, glm::vec3(1.0f, 0.9f, 0.3f), 0.35f, 1.3f);
    }
}

void EditorGizmoSystem::DrawDirectionalLight(const GameObject& obj, const glm::mat4& worldMat) {
    glm::vec3 pos = glm::vec3(worldMat[3]);

    glm::vec3 localDir = obj.light.direction;
    if (glm::length(localDir) < 1e-4f) localDir = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 fwd = glm::normalize(glm::vec3(worldMat * glm::vec4(localDir, 0.0f)));

    glm::vec3 sunColor(1.0f, 0.92f, 0.25f);

    // Position marker: circular sun ring and rays
    DrawCircle(pos, fwd, 0.45f, sunColor, 24, 1.25f);

    // Center forward direction arrow
    DrawArrow(pos, pos + fwd * 4.2f, sunColor, 0.45f, 1.5f);

    // Parallel direction rays/arrows communicating infinite directional illumination
    if (showLightDirections) {
        glm::vec3 ref = std::abs(fwd.y) < 0.99f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 side = glm::normalize(glm::cross(fwd, ref));
        glm::vec3 up = glm::normalize(glm::cross(side, fwd));

        // 4 orthogonal parallel arrows
        float dist1 = 1.35f;
        glm::vec3 offsets1[4] = {
            side * dist1,
            -side * dist1,
            up * dist1,
            -up * dist1
        };
        for (int i = 0; i < 4; ++i) {
            glm::vec3 p = pos + offsets1[i];
            DrawArrow(p, p + fwd * 3.4f, sunColor * 0.85f, 0.35f, 1.1f);
        }

        // 4 diagonal parallel arrows at larger radius
        float dist2 = 2.1f;
        const float invSqrt2 = 0.70710678f;
        glm::vec3 offsets2[4] = {
            (side + up) * (dist2 * invSqrt2),
            (-side + up) * (dist2 * invSqrt2),
            (-side - up) * (dist2 * invSqrt2),
            (side - up) * (dist2 * invSqrt2)
        };
        for (int i = 0; i < 4; ++i) {
            glm::vec3 p = pos + offsets2[i];
            DrawArrow(p, p + fwd * 2.8f, sunColor * 0.65f, 0.30f, 0.9f);
        }
    }
}

void EditorGizmoSystem::DrawAreaLight(const GameObject& obj, const glm::mat4& worldMat) {
    glm::vec3 pos = glm::vec3(worldMat[3]);

    glm::vec3 localDir = obj.light.direction;
    if (glm::length(localDir) < 1e-4f) localDir = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 fwd = glm::normalize(glm::vec3(worldMat * glm::vec4(localDir, 0.0f)));

    glm::vec3 uAxis = glm::normalize(glm::vec3(worldMat[0]));
    glm::vec3 vAxis = glm::normalize(glm::vec3(worldMat[1]));

    glm::vec3 areaColor(0.35f, 0.85f, 1.0f);

    // Light emitter surface
    if (obj.light.areaShape == 0) { // Rectangle
        DrawRectangle(pos, uAxis, vAxis, obj.light.width * 0.5f, obj.light.height * 0.5f, areaColor, 1.25f);
    } else { // Disk / Circle
        DrawCircle(pos, fwd, obj.light.radius, areaColor, 36, 1.25f);
    }

    // Forward direction arrow
    if (showLightDirections) {
        DrawArrow(pos, pos + fwd * 2.0f, areaColor, 0.35f, 1.3f);
    }

    // Range boundary box if finite range configured
    if (showLightRanges && obj.light.range > 0.01f) {
        glm::vec3 extents(obj.light.width, obj.light.height, obj.light.range * 2.0f);
        DrawBox(pos + fwd * (obj.light.range * 0.5f), extents, worldMat, areaColor * 0.45f, 0.8f);
    }
}

void EditorGizmoSystem::DrawCameraFrustum(const GameObject& obj, const glm::mat4& worldMat, bool isActiveCamera) {
    glm::vec3 pos = glm::vec3(worldMat[3]);

    // Local forward is (0, 0, -1) and local up is (0, 1, 0)
    glm::vec3 fwd = glm::normalize(glm::vec3(worldMat * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
    glm::vec3 up = glm::normalize(glm::vec3(worldMat * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));

    glm::vec3 frustumColor = isActiveCamera ? glm::vec3(0.1f, 1.0f, 0.35f) : glm::vec3(0.25f, 0.85f, 1.0f);
    float thickness = isActiveCamera ? 1.5f : 1.15f;

    float aspect = obj.camera.aspectRatio > 0.01f ? obj.camera.aspectRatio : (16.0f / 9.0f);
    float fov = std::clamp(obj.camera.fov, 10.0f, 150.0f);
    float nearPlane = std::max(0.1f, obj.camera.nearPlane);
    float farPlane = std::clamp(obj.camera.farPlane, nearPlane + 0.5f, 35.0f);

    DrawFrustum(pos, fwd, up, fov, aspect, nearPlane, farPlane, frustumColor, showClipPlanes, thickness);
}

void EditorGizmoSystem::RenderGizmos(const Scene& scene, const glm::vec3& cameraPos,
                                     std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices,
                                     std::vector<RenderBatch>& outBatches)
{
    // Gizmos must NEVER execute or render during normal game runtime (dev.md Section 1, 11)
    if (scene.isPlayMode) return;

    m_currentVertices = &outVertices;
    m_currentIndices = &outIndices;
    m_cameraPos = cameraPos;

    uint32_t startIdx = (uint32_t)outIndices.size();

    // 1. Draw Gizmo for currently selected object
    if (scene.selectedId != -1) {
        const GameObject* selObj = scene.FindObject(scene.selectedId);
        if (selObj && selObj->visible) {
            glm::mat4 worldMat = scene.GetWorldMatrix(*selObj);

            if (selObj->isLight || IsLightPrimitive(selObj->type)) {
                if (showLightGizmos) {
                    LightType lType = selObj->light.type;
                    if (selObj->type == PrimitiveType::DirectionalLight) lType = LightType::Directional;
                    else if (selObj->type == PrimitiveType::PointLight) lType = LightType::Point;
                    else if (selObj->type == PrimitiveType::SpotLight) lType = LightType::Spot;
                    else if (selObj->type == PrimitiveType::AreaLight) lType = LightType::Area;

                    switch (lType) {
                        case LightType::Point:       DrawPointLight(*selObj, worldMat); break;
                        case LightType::Spot:        DrawSpotLight(*selObj, worldMat); break;
                        case LightType::Directional: DrawDirectionalLight(*selObj, worldMat); break;
                        case LightType::Area:        DrawAreaLight(*selObj, worldMat); break;
                        default: break;
                    }
                }
            } else if (selObj->isCamera || IsCameraPrimitive(selObj->type)) {
                if (showCameraGizmos) {
                    bool isActiveCam = (scene.activeLevelCameraId == selObj->id);
                    DrawCameraFrustum(*selObj, worldMat, isActiveCam);
                }
            }
        }
    }

    // 2. Active Level Camera frustum (when different from selected object)
    if (showCameraGizmos && scene.activeLevelCameraId != -1 && scene.activeLevelCameraId != scene.selectedId) {
        const GameObject* actCam = scene.FindObject(scene.activeLevelCameraId);
        if (actCam && actCam->visible) {
            glm::mat4 worldMat = scene.GetWorldMatrix(*actCam);
            DrawCameraFrustum(*actCam, worldMat, true);
        }
    }

    uint32_t count = (uint32_t)outIndices.size() - startIdx;
    if (count > 0) {
        RenderBatch b;
        b.startIndex = startIdx;
        b.indexCount = count;
        b.isUnlit = true;
        b.castShadows = false;
        b.receiveShadows = false;
        b.baseColor = {1.0f, 1.0f, 1.0f};
        outBatches.push_back(b);
    }

    m_currentVertices = nullptr;
    m_currentIndices = nullptr;
}

} // namespace Eunoia
