#pragma once

struct ID3D12Device;
struct ID3D12CommandQueue;

class IRHI {
public:
    virtual ~IRHI() = default;
    virtual bool Initialize(void* windowHandle, int width, int height) = 0;
    virtual void Shutdown() = 0;
    virtual ID3D12Device* GetDevice() const = 0;
    virtual ID3D12CommandQueue* GetCommandQueue() const = 0;
};
