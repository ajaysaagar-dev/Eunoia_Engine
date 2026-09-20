#pragma once
// ============================================================================
// EngineRHI::SwapChain — DXGI swapchain management
// ============================================================================

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdint>

namespace EngineRHI {

using Microsoft::WRL::ComPtr;

struct SwapChainConfig {
    HWND     hwnd         = nullptr;
    uint32_t width        = 1280;
    uint32_t height       = 720;
    uint32_t frameCount   = 2;
    DXGI_FORMAT format    = DXGI_FORMAT_R8G8B8A8_UNORM;
};

class SwapChain {
public:
    bool Initialize(IDXGIFactory4* factory, ID3D12CommandQueue* cmdQueue,
                    ID3D12Device* device, const SwapChainConfig& cfg);
    void Shutdown();

    // Returns false on DXGI_ERROR_DEVICE_REMOVED or other present errors
    bool Present(uint32_t syncInterval = 1);

    void Resize(uint32_t width, uint32_t height);

    uint32_t          CurrentBackBufferIndex() const;
    ID3D12Resource*   BackBuffer(uint32_t index) const;
    D3D12_CPU_DESCRIPTOR_HANDLE RTV(uint32_t index) const;

    IDXGISwapChain3* Get() const { return m_swapChain.Get(); }

private:
    ComPtr<IDXGISwapChain3>        m_swapChain;
    ComPtr<ID3D12DescriptorHeap>   m_rtvHeap;
    ComPtr<ID3D12Resource>         m_backBuffers[3]; // max 3 frames
    uint32_t                       m_frameCount   = 2;
    uint32_t                       m_rtvDescSize  = 0;
};

} // namespace EngineRHI
