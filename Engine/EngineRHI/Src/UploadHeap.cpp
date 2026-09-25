#include <EngineRHI/UploadHeap.h>
#include <iostream>

namespace EngineRHI {

bool UploadHeap::Initialize(ID3D12Device* device, ID3D12CommandQueue* queue) {
    if (!device || !queue) return false;

    m_queue = queue;

    HRESULT hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_alloc));
    if (FAILED(hr)) return false;

    hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_alloc.Get(), nullptr, IID_PPV_ARGS(&m_list));
    if (FAILED(hr)) return false;
    m_list->Close();

    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
    if (FAILED(hr)) return false;

    m_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_event) return false;

    m_fenceValue = 0;
    return true;
}

void UploadHeap::Shutdown() {
    if (m_event) {
        CloseHandle(m_event);
        m_event = nullptr;
    }
    m_fence.Reset();
    m_list.Reset();
    m_alloc.Reset();
    m_queue.Reset();
}

UploadTextureResult UploadHeap::UploadTexture(
    const uint8_t*       pixels,
    int                  width,
    int                  height,
    DXGI_FORMAT          format,
    ID3D12DescriptorHeap* srvHeap,
    uint32_t              heapOffset,
    uint32_t              descriptorSize
) {
    UploadTextureResult res{};
    if (!pixels || width <= 0 || height <= 0 || !m_queue || !m_alloc || !m_list) {
        return res;
    }

    ComPtr<ID3D12Device> device;
    m_queue->GetDevice(IID_PPV_ARGS(&device));
    if (!device) return res;

    // Align row pitch to 256 bytes
    UINT rowPitch = (width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
    UINT64 uploadSize = static_cast<UINT64>(rowPitch) * height;

    // Create GPU default texture
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = format;
    texDesc.SampleDesc.Count = 1;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    D3D12_HEAP_PROPERTIES defaultHeap = {};
    defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

    ComPtr<ID3D12Resource> texture;
    HRESULT hr = device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&texture)
    );
    if (FAILED(hr)) return res;

    // Create intermediate upload buffer
    D3D12_HEAP_PROPERTIES uploadHeap = {};
    uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufDesc = {};
    bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufDesc.Width = uploadSize;
    bufDesc.Height = 1;
    bufDesc.DepthOrArraySize = 1;
    bufDesc.MipLevels = 1;
    bufDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufDesc.SampleDesc.Count = 1;
    bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ComPtr<ID3D12Resource> uploadBuffer;
    hr = device->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &bufDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadBuffer)
    );
    if (FAILED(hr)) return res;

    // Copy CPU pixels to upload buffer with pitch alignment
    uint8_t* mappedData = nullptr;
    hr = uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
    if (FAILED(hr)) return res;

    for (int y = 0; y < height; ++y) {
        memcpy(mappedData + static_cast<size_t>(y) * rowPitch,
               pixels + static_cast<size_t>(y) * width * 4,
               width * 4);
    }
    uploadBuffer->Unmap(0, nullptr);

    // Record copy
    m_alloc->Reset();
    m_list->Reset(m_alloc.Get(), nullptr);

    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource = texture.Get();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource = uploadBuffer.Get();
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint.Offset = 0;
    src.PlacedFootprint.Footprint.Format = format;
    src.PlacedFootprint.Footprint.Width = width;
    src.PlacedFootprint.Footprint.Height = height;
    src.PlacedFootprint.Footprint.Depth = 1;
    src.PlacedFootprint.Footprint.RowPitch = rowPitch;

    m_list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    // Transition to PIXEL_SHADER_RESOURCE
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = texture.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_list->ResourceBarrier(1, &barrier);

    m_list->Close();

    // Execute copy
    ID3D12CommandList* cmdLists[] = { m_list.Get() };
    m_queue->ExecuteCommandLists(1, cmdLists);

    // Wait on fence
    m_fenceValue++;
    m_queue->Signal(m_fence.Get(), m_fenceValue);
    if (m_fence->GetCompletedValue() < m_fenceValue) {
        m_fence->SetEventOnCompletion(m_fenceValue, m_event);
        WaitForSingleObject(m_event, INFINITE);
    }

    // Create SRV in the descriptor heap if requested
    if (srvHeap) {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
        cpuHandle.ptr += static_cast<SIZE_T>(heapOffset) * descriptorSize;
        device->CreateShaderResourceView(texture.Get(), &srvDesc, cpuHandle);
        res.srvIndex = heapOffset;
    }

    res.texture = texture;
    res.valid = true;
    return res;
}

bool UploadHeap::Flush() {
    if (!m_queue || !m_fence) return false;
    m_fenceValue++;
    m_queue->Signal(m_fence.Get(), m_fenceValue);
    if (m_fence->GetCompletedValue() < m_fenceValue) {
        m_fence->SetEventOnCompletion(m_fenceValue, m_event);
        WaitForSingleObject(m_event, INFINITE);
    }
    return true;
}

} // namespace EngineRHI
