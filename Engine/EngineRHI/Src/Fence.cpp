#include <EngineRHI/Fence.h>
#include <iostream>

namespace EngineRHI {

FenceWaitResult SafeWaitForFence(
    ID3D12Fence* fence,
    UINT64       targetValue,
    HANDLE       eventHandle,
    UINT32       timeoutMs
) {
    FenceWaitResult result;
    if (!fence || !eventHandle) {
        result.ok = false;
        return result;
    }

    if (fence->GetCompletedValue() >= targetValue) {
        result.ok = true;
        return result;
    }

    fence->SetEventOnCompletion(targetValue, eventHandle);
    DWORD waitRes = WaitForSingleObject(eventHandle, timeoutMs == UINT32_MAX ? INFINITE : timeoutMs);

    if (waitRes == WAIT_TIMEOUT) {
        result.ok = false;
        result.deviceLost = false;
        return result;
    }

    result.ok = (waitRes == WAIT_OBJECT_0);
    return result;
}

bool CreateFenceAndEvent(
    ID3D12Device* device,
    ComPtr<ID3D12Fence>& outFence,
    HANDLE&              outEvent,
    UINT64               initialValue
) {
    if (!device) return false;

    HRESULT hr = device->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&outFence));
    if (FAILED(hr)) {
        std::cerr << "[EngineRHI] Failed to create D3D12 fence: " << hr << std::endl;
        return false;
    }

    outEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!outEvent) {
        std::cerr << "[EngineRHI] Failed to create Win32 event for fence!" << std::endl;
        return false;
    }

    return true;
}

} // namespace EngineRHI
