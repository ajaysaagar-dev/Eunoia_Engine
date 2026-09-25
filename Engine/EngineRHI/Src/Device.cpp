#include <EngineRHI/Device.h>
#include <iostream>
#include <vector>

namespace EngineRHI {

bool Device::Initialize(const DeviceConfig& cfg) {
    UINT dxgiFactoryFlags = 0;

    if (cfg.enableDebugLayer) {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            debugController->EnableDebugLayer();

            if (cfg.enableGPUValidation) {
                ComPtr<ID3D12Debug1> debug1;
                if (SUCCEEDED(debugController.As(&debug1))) {
                    debug1->SetEnableGPUBasedValidation(TRUE);
                }
            }
            std::cout << "[EngineRHI] D3D12 debug layer enabled." << std::endl;
        }
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }

    HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory));
    if (FAILED(hr)) {
        std::cerr << "[EngineRHI] Failed to create DXGIFactory2: " << hr << std::endl;
        return false;
    }

    // Enumerate hardware adapters
    ComPtr<IDXGIAdapter1> chosenAdapter;
    ComPtr<IDXGIAdapter1> adapter;
    uint32_t currentAdapterIndex = 0;

    for (UINT i = 0; m_factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            continue;
        }

        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr))) {
            if (currentAdapterIndex == cfg.adapterIndex || chosenAdapter == nullptr) {
                chosenAdapter = adapter;
                if (currentAdapterIndex == cfg.adapterIndex) {
                    break;
                }
            }
            currentAdapterIndex++;
        }
    }

    if (!chosenAdapter) {
        std::cerr << "[EngineRHI] No suitable D3D12 hardware adapter found!" << std::endl;
        return false;
    }

    m_adapter = chosenAdapter;

    hr = D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));
    if (FAILED(hr)) {
        std::cerr << "[EngineRHI] Failed to create D3D12Device: " << hr << std::endl;
        return false;
    }

    std::cout << "[EngineRHI] D3D12 Device initialized successfully." << std::endl;
    return true;
}

void Device::Shutdown() {
    m_device.Reset();
    m_adapter.Reset();
    m_factory.Reset();
}

} // namespace EngineRHI
