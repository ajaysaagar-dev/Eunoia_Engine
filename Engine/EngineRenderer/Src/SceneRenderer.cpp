#include <EngineRenderer/SceneRenderer.h>
#include <EngineScene/Scene.h>
#include <iostream>

namespace EngineRenderer {

bool SceneRenderer::Initialize(ID3D12Device* device, const SceneRendererConfig& cfg) {
    if (!device) return false;
    m_cfg = cfg;
    m_initialized = true;
    return true;
}

void SceneRenderer::Shutdown() {
    m_initialized = false;
}

void SceneRenderer::RenderFrame(
    Scene&              scene,
    const OrbitCamera&  camera,
    ID3D12GraphicsCommandList* cmdList,
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
    uint32_t            viewportWidth,
    uint32_t            viewportHeight
) {
    if (!m_initialized || !cmdList) return;

    D3D12_VIEWPORT vp = {};
    vp.Width = static_cast<float>(viewportWidth);
    vp.Height = static_cast<float>(viewportHeight);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    D3D12_RECT scissor = { 0, 0, static_cast<LONG>(viewportWidth), static_cast<LONG>(viewportHeight) };

    cmdList->RSSetViewports(1, &vp);
    cmdList->RSSetScissorRects(1, &scissor);

    cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
}

void SceneRenderer::UpdateSceneGeometry(Scene& scene) {
    // Rebuild geometry if needed
}

} // namespace EngineRenderer
