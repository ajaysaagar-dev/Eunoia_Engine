#pragma once
// ============================================================================
// EngineRHI::Device — D3D12 adapter enumeration and device creation
// ============================================================================

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdint>

namespace EngineRHI {

using Microsoft::WRL::ComPtr;

struct DeviceConfig {
    bool enableDebugLayer  = true;  // disabled in NDEBUG / release
    bool enableGPUValidation = false; // expensive, off by default
    uint32_t adapterIndex  = 0;     // 0 = prefer highest-perf adapter
};

class Device {
public:
    Device()  = default;
    ~Device() = default;

    bool Initialize(const DeviceConfig& cfg = {});
    void Shutdown();

    ID3D12Device*       Get()     const { return m_device.Get(); }
    IDXGIFactory4*      Factory() const { return m_factory.Get(); }
    IDXGIAdapter1*      Adapter() const { return m_adapter.Get(); }

    bool IsValid() const { return m_device != nullptr; }

private:
    ComPtr<ID3D12Device>  m_device;
    ComPtr<IDXGIFactory4> m_factory;
    ComPtr<IDXGIAdapter1> m_adapter;
};

} // namespace EngineRHI
