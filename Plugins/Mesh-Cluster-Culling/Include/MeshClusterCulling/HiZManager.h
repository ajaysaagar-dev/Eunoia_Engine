#pragma once
// ============================================================================
// Eunoia Engine — Mesh Cluster Culling Plugin
// HiZManager: GPU Hierarchical-Z Depth Pyramid Generation
// ============================================================================

#include <d3d12.h>
#include <dxgi1_6.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace Eunoia {

class HiZManager {
public:
    HiZManager();
    ~HiZManager();

    bool Initialize(ID3D12Device* device, UINT width, UINT height);
    void Shutdown();
    void Resize(ID3D12Device* device, UINT width, UINT height);

    // Builds the complete Hi-Z mip chain using compute shader
    void GeneratePyramid(
        ID3D12GraphicsCommandList* cmdList,
        ID3D12Resource* sceneDepthBuffer,
        D3D12_RESOURCE_STATES currentDepthState
    );

    ID3D12Resource* GetHiZTexture() const { return m_hizTexture; }
    UINT GetWidth() const { return m_width; }
    UINT GetHeight() const { return m_height; }
    UINT GetMipLevels() const { return m_mipLevels; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvGpuHandle() const { return m_srvGpuHandle; }
    bool IsReady() const { return m_isReady; }

private:
    bool CreateResources(ID3D12Device* device);
    bool CreatePipelines(ID3D12Device* device);
    void ReleaseResources();

    ID3D12Device*             m_device = nullptr;
    ID3D12Resource*           m_hizTexture = nullptr;
    ID3D12DescriptorHeap*     m_descriptorHeap = nullptr; // SRV & UAVs for mips
    UINT                      m_descriptorSize = 0;

    ID3D12RootSignature*      m_rootSignature = nullptr;
    ID3D12PipelineState*      m_hizBuildPSO = nullptr;

    UINT                      m_width = 0;
    UINT                      m_height = 0;
    UINT                      m_mipLevels = 0;
    bool                      m_isReady = false;

    D3D12_GPU_DESCRIPTOR_HANDLE m_srvGpuHandle = {};
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_uavCpuHandles;
    std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> m_uavGpuHandles;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_srvMipCpuHandles;
    std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> m_srvMipGpuHandles;
};

} // namespace Eunoia
