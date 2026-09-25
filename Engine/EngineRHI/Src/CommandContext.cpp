#include <EngineRHI/CommandContext.h>
#include <iostream>

namespace EngineRHI {

bool CommandContext::Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type,
                                const wchar_t* debugName) {
    if (!device) return false;

    HRESULT hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&m_alloc));
    if (FAILED(hr)) {
        std::cerr << "[EngineRHI] Failed to create command allocator: " << hr << std::endl;
        return false;
    }

    hr = device->CreateCommandList(0, type, m_alloc.Get(), nullptr, IID_PPV_ARGS(&m_list));
    if (FAILED(hr)) {
        std::cerr << "[EngineRHI] Failed to create command list: " << hr << std::endl;
        return false;
    }

    // Command lists are created in recording state; close it initially
    m_list->Close();

    if (debugName) {
        m_alloc->SetName(debugName);
        m_list->SetName(debugName);
    }

    return true;
}

void CommandContext::Shutdown() {
    m_list.Reset();
    m_alloc.Reset();
}

bool CommandContext::Reset(ID3D12PipelineState* pso) {
    if (!m_alloc || !m_list) return false;

    HRESULT hr = m_alloc->Reset();
    if (FAILED(hr)) {
        std::cerr << "[EngineRHI] Failed to reset command allocator: " << hr << std::endl;
        return false;
    }

    hr = m_list->Reset(m_alloc.Get(), pso);
    if (FAILED(hr)) {
        std::cerr << "[EngineRHI] Failed to reset command list: " << hr << std::endl;
        return false;
    }

    return true;
}

bool CommandContext::Close() {
    if (!m_list) return false;
    HRESULT hr = m_list->Close();
    if (FAILED(hr)) {
        std::cerr << "[EngineRHI] Failed to close command list: " << hr << std::endl;
        return false;
    }
    return true;
}

} // namespace EngineRHI
