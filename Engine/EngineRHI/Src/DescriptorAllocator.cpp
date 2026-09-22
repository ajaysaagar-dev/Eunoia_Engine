#include <EngineRHI/DescriptorAllocator.h>

namespace EngineRHI {

bool DescriptorAllocator::Initialize(ID3D12DescriptorHeap* heap, uint32_t capacity, uint32_t descriptorSize) {
    if (!heap || capacity == 0 || descriptorSize == 0) {
        return false;
    }

    m_heap = heap;
    m_capacity = capacity;
    m_descriptorSize = descriptorSize;
    m_cpuBase = heap->GetCPUDescriptorHandleForHeapStart();
    
    D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
    if (desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) {
        m_gpuBase = heap->GetGPUDescriptorHandleForHeapStart();
    } else {
        m_gpuBase = {};
    }

    m_head.store(0);
    return true;
}

DescriptorAllocator::Allocation DescriptorAllocator::Allocate() {
    uint32_t idx = m_head.fetch_add(1);
    if (idx >= m_capacity) {
        Allocation empty{};
        empty.valid = false;
        return empty;
    }

    Allocation alloc{};
    alloc.index = idx;
    alloc.valid = true;

    alloc.cpu = m_cpuBase;
    alloc.cpu.ptr += static_cast<SIZE_T>(idx) * m_descriptorSize;

    if (m_gpuBase.ptr != 0) {
        alloc.gpu = m_gpuBase;
        alloc.gpu.ptr += static_cast<UINT64>(idx) * m_descriptorSize;
    } else {
        alloc.gpu = {};
    }

    return alloc;
}

void DescriptorAllocator::Reset() {
    m_head.store(0);
}

} // namespace EngineRHI
