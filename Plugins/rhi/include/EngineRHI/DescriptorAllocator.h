#pragma once
// ============================================================================
// EngineRHI::DescriptorAllocator — bounds-checked linear SRV/CBV/UAV allocator
//
// Replaces the scattered g_srvDescHeap linear counter in Cube.cpp.
// A single allocator instance is owned by the renderer; callers use
// Allocate() to get a CPU+GPU descriptor handle pair.
//
// Future: add a FreeList path for descriptors that need to be recycled
// (e.g. when a texture is deleted).
// ============================================================================

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <atomic>

namespace EngineRHI {

using Microsoft::WRL::ComPtr;

class DescriptorAllocator {
public:
    // Initialize with a pre-created descriptor heap.
    // capacity: total number of descriptors the heap can hold.
    bool Initialize(ID3D12DescriptorHeap* heap, uint32_t capacity, uint32_t descriptorSize);

    struct Allocation {
        D3D12_CPU_DESCRIPTOR_HANDLE cpu{};
        D3D12_GPU_DESCRIPTOR_HANDLE gpu{};
        uint32_t                    index = UINT32_MAX;
        bool                        valid = false;
        explicit operator bool() const { return valid; }
    };

    // Linear allocate — never fails if heap has space.
    Allocation Allocate();

    uint32_t Capacity()  const { return m_capacity; }
    uint32_t Used()      const { return m_head.load(); }
    uint32_t Remaining() const { return m_capacity - m_head.load(); }

    void Reset(); // reclaim all (use with care — GPU must have finished)

private:
    ID3D12DescriptorHeap*       m_heap           = nullptr;
    uint32_t                    m_capacity        = 0;
    uint32_t                    m_descriptorSize  = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE m_cpuBase{};
    D3D12_GPU_DESCRIPTOR_HANDLE m_gpuBase{};
    std::atomic<uint32_t>       m_head{0};
};

} // namespace EngineRHI
