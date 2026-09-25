#include <EngineRHI/SwapChain.h>
#include <iostream>

namespace EngineRHI {

bool SwapChain::Initialize(IDXGIFactory4* factory, ID3D12CommandQueue* cmdQueue,
                           ID3D12Device* device, const SwapChainConfig& cfg) {
    if (!factory || !cmdQueue || !device || !cfg.hwnd) {
        return false;
    }

    m_frameCount = std::min(cfg.frameCount, 3u);

    // 1. Create RTV descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = m_frameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    HRESULT hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
    if (FAILED(hr)) {
        std::cerr << "[EngineRHI] Failed to create RTV descriptor heap for SwapChain: " << hr << std::endl;
        return false;
    }
    m_rtvDescSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // 2. Create SwapChain
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = m_frameCount;
    swapChainDesc.Width = cfg.width;
    swapChainDesc.Height = cfg.height;
    swapChainDesc.Format = cfg.format;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> sc1;
    hr = factory->CreateSwapChainForHwnd(cmdQueue, cfg.hwnd, &swapChainDesc, nullptr, nullptr, &sc1);
    if (FAILED(hr)) {
        std::cerr << "[EngineRHI] Failed to create SwapChain for HWND: " << hr << std::endl;
        return false;
    }

    hr = sc1.As(&m_swapChain);
    if (FAILED(hr)) {
        std::cerr << "[EngineRHI] Failed to query IDXGISwapChain3: " << hr << std::endl;
        return false;
    }

    // 3. Create RTVs
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (uint32_t i = 0; i < m_frameCount; ++i) {
        hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i]));
        if (FAILED(hr)) {
            std::cerr << "[EngineRHI] Failed to get backbuffer " << i << ": " << hr << std::endl;
            return false;
        }
        device->CreateRenderTargetView(m_backBuffers[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += m_rtvDescSize;
    }

    return true;
}

void SwapChain::Shutdown() {
    for (uint32_t i = 0; i < 3; ++i) {
        m_backBuffers[i].Reset();
    }
    m_rtvHeap.Reset();
    m_swapChain.Reset();
}

bool SwapChain::Present(uint32_t syncInterval) {
    if (!m_swapChain) return false;
    HRESULT hr = m_swapChain->Present(syncInterval, 0);
    if (FAILED(hr)) {
        std::cerr << "[EngineRHI] SwapChain::Present failed: 0x" << std::hex << hr << std::dec << std::endl;
        return false;
    }
    return true;
}

void SwapChain::Resize(uint32_t width, uint32_t height) {
    if (!m_swapChain || width == 0 || height == 0) return;

    for (uint32_t i = 0; i < m_frameCount; ++i) {
        m_backBuffers[i].Reset();
    }

    HRESULT hr = m_swapChain->ResizeBuffers(m_frameCount, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    if (FAILED(hr)) {
        std::cerr << "[EngineRHI] SwapChain::Resize failed: " << hr << std::endl;
        return;
    }

    // Retrieve backbuffers
    ComPtr<ID3D12Device> device;
    m_swapChain->GetDevice(IID_PPV_ARGS(&device));
    if (device && m_rtvHeap) {
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        for (uint32_t i = 0; i < m_frameCount; ++i) {
            m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i]));
            device->CreateRenderTargetView(m_backBuffers[i].Get(), nullptr, rtvHandle);
            rtvHandle.ptr += m_rtvDescSize;
        }
    }
}

uint32_t SwapChain::CurrentBackBufferIndex() const {
    return m_swapChain ? m_swapChain->GetCurrentBackBufferIndex() : 0;
}

ID3D12Resource* SwapChain::BackBuffer(uint32_t index) const {
    if (index < m_frameCount) {
        return m_backBuffers[index].Get();
    }
    return nullptr;
}

D3D12_CPU_DESCRIPTOR_HANDLE SwapChain::RTV(uint32_t index) const {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
    if (m_rtvHeap && index < m_frameCount) {
        handle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += static_cast<SIZE_T>(index) * m_rtvDescSize;
    }
    return handle;
}

} // namespace EngineRHI
