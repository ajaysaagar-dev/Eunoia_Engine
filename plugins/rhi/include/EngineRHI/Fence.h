#pragma once
// ============================================================================
// EngineRHI::Fence — GPU fence utilities and device-loss detection
//
// SafeWaitForFence replaces the raw fence-wait scattered across Cube.cpp.
// On device removal (DXGI_ERROR_DEVICE_REMOVED / DEVICE_HUNG) it sets
// *outDeviceLost = true so the caller can stop rendering.
// ============================================================================

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

namespace EngineRHI {

using Microsoft::WRL::ComPtr;

struct FenceWaitResult {
    bool ok        = true;  // false = timeout or device lost
    bool deviceLost = false;
};

/// Wait for fence to reach or exceed targetValue.
/// timeoutMs : 0 = poll (non-blocking).  UINT32_MAX = infinite.
FenceWaitResult SafeWaitForFence(
    ID3D12Fence* fence,
    UINT64       targetValue,
    HANDLE       eventHandle,
    UINT32       timeoutMs = 5000
);

/// Create a fence + a Win32 auto-reset event. Returns false on failure.
bool CreateFenceAndEvent(
    ID3D12Device* device,
    ComPtr<ID3D12Fence>& outFence,
    HANDLE&              outEvent,
    UINT64               initialValue = 0
);

} // namespace EngineRHI
