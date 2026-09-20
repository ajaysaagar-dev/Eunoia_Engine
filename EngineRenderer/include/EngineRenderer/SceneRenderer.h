#pragma once
// ============================================================================
// EngineRenderer::SceneRenderer — shadow pass + main PBR pass
//
// Extracted from the inline renderFrame() in Cube.cpp.
// Depends on EngineRHI for D3D12 types and EngineScene for Scene/Camera.
// Does NOT know about ImGui.
// ============================================================================

#include <d3d12.h>
#include <wrl/client.h>
#include "../../include/Scene.h"
#include "../../include/Camera.h"

namespace EngineRenderer {

using Microsoft::WRL::ComPtr;

struct SceneRendererConfig {
    uint32_t shadowMapWidth  = 2048;
    uint32_t shadowMapHeight = 2048;
    uint32_t maxVertices     = 1'000'000;
    uint32_t maxIndices      = 3'000'000;
};

class SceneRenderer {
public:
    bool Initialize(ID3D12Device* device, const SceneRendererConfig& cfg = {});
    void Shutdown();

    // Call once per frame after scene geometry has been updated.
    void RenderFrame(
        Scene&              scene,
        const OrbitCamera&  camera,
        ID3D12GraphicsCommandList* cmdList,
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
        uint32_t            viewportWidth,
        uint32_t            viewportHeight
    );

    // Explicit geometry rebuild (e.g. if scene changed outside a frame)
    void UpdateSceneGeometry(Scene& scene);

    bool IsValid() const { return m_initialized; }

private:
    bool m_initialized = false;
    SceneRendererConfig m_cfg;
};

} // namespace EngineRenderer
