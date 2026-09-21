#pragma once
// ============================================================================
// EngineRenderer::ConstantBuffers — shared CB structs for shaders
// ============================================================================

#include <glm/glm.hpp>

namespace EngineRenderer {

// Per-frame constant buffer layout (matches HLSL cbuffer SceneConstants)
struct alignas(256) SceneConstantBuffer {
    glm::mat4 viewProj;
    glm::mat4 lightViewProj;
    glm::vec3 lightDir;
    float     lightIntensity;
    glm::vec3 lightColor;
    float     ambientIntensity;
    glm::vec3 cameraPos;
    float     shadowStrength;
    float     shadowBias;
    float     pcfRadius;
    int       enableShadows;
    float     _pad0;
    glm::vec4 clearColor;
};

} // namespace EngineRenderer
