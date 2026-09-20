#pragma once
// ============================================================================
// EngineRHI::UploadHeap — async texture upload pipeline
//
// Wraps the separate upload command allocator / list / fence used in Cube.cpp
// so that texture decoding (stb_image) can be moved off the render thread
// using EngineCore::JobSystem in a future pass.
// ============================================================================

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <string>
#include <vector>

namespace EngineRHI {

using Microsoft::WRL::ComPtr;

struct UploadTextureResult {
    ComPtr<ID3D12Resource> texture;
    uint32_t               srvIndex = UINT32_MAX; // index in the SRV heap
    bool                   valid    = false;
};

class UploadHeap {
public:
    bool Initialize(ID3D12Device* device, ID3D12CommandQueue* queue);
    void Shutdown();

    // Upload raw RGBA pixel data.
    // srvHeap / heapOffset: where to write the SRV descriptor.
    UploadTextureResult UploadTexture(
        const uint8_t*       pixels,
        int                  width,
        int                  height,
        DXGI_FORMAT          format,
        ID3D12DescriptorHeap* srvHeap,
        uint32_t              heapOffset,
        uint32_t              descriptorSize
    );

    // Flush all pending uploads and wait for the GPU to finish.
    bool Flush();

    uint64_t NextFenceValue() const { return m_fenceValue + 1; }

private:
    ComPtr<ID3D12CommandAllocator>    m_alloc;
    ComPtr<ID3D12GraphicsCommandList> m_list;
    ComPtr<ID3D12Fence>               m_fence;
    ComPtr<ID3D12CommandQueue>        m_queue;
    HANDLE                            m_event = nullptr;
    uint64_t                          m_fenceValue = 0;
};

} // namespace EngineRHI
