#pragma once
// ============================================================================
// EngineRHI::CommandContext — command list / allocator helpers
// ============================================================================

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

namespace EngineRHI {

using Microsoft::WRL::ComPtr;

class CommandContext {
public:
    bool Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type,
                    const wchar_t* debugName = nullptr);
    void Shutdown();

    // Reset allocator and re-open the list (call at frame start)
    bool Reset(ID3D12PipelineState* pso = nullptr);

    // Close the list for submission
    bool Close();

    ID3D12GraphicsCommandList* List() const { return m_list.Get(); }
    ID3D12CommandAllocator*    Alloc() const { return m_alloc.Get(); }

private:
    ComPtr<ID3D12GraphicsCommandList> m_list;
    ComPtr<ID3D12CommandAllocator>    m_alloc;
};

} // namespace EngineRHI
