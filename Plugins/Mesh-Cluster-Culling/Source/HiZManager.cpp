// ============================================================================
// Eunoia Engine — Mesh Cluster Culling Plugin
// HiZManager: Implementation of GPU Hierarchical-Z Depth Pyramid Generation
// ============================================================================

#include "MeshClusterCulling/HiZManager.h"
#include <d3dcompiler.h>
#include <iostream>
#include <algorithm>
#include <cmath>

namespace Eunoia {

static inline UINT NextPowerOfTwo(UINT v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

HiZManager::HiZManager() = default;

HiZManager::~HiZManager() {
    Shutdown();
}

bool HiZManager::Initialize(ID3D12Device* device, UINT width, UINT height) {
    if (!device) return false;
    m_device = device;

    // Hi-Z dimension: power-of-two (e.g. 512x512 or 1024x1024)
    m_width = std::min(1024u, NextPowerOfTwo(width));
    m_height = std::min(1024u, NextPowerOfTwo(height));
    if (m_width == 0) m_width = 512;
    if (m_height == 0) m_height = 512;

    UINT maxDim = std::max(m_width, m_height);
    m_mipLevels = (UINT)std::floor(std::log2((float)maxDim)) + 1;

    m_descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    if (!CreatePipelines(device)) {
        std::cerr << "[MeshClusterCulling] Failed to create Hi-Z compute pipeline." << std::endl;
        return false;
    }

    if (!CreateResources(device)) {
        std::cerr << "[MeshClusterCulling] Failed to allocate Hi-Z resources." << std::endl;
        return false;
    }

    m_isReady = true;
    std::cout << "[MeshClusterCulling] Hi-Z pyramid initialized: " << m_width << "x" << m_height
              << " (" << m_mipLevels << " mips)" << std::endl;
    return true;
}

void HiZManager::Shutdown() {
    ReleaseResources();
    if (m_hizBuildPSO) { m_hizBuildPSO->Release(); m_hizBuildPSO = nullptr; }
    if (m_rootSignature) { m_rootSignature->Release(); m_rootSignature = nullptr; }
    m_isReady = false;
}

void HiZManager::Resize(ID3D12Device* device, UINT width, UINT height) {
    if (!device) return;
    UINT newW = std::min(1024u, NextPowerOfTwo(width));
    UINT newH = std::min(1024u, NextPowerOfTwo(height));
    if (newW == m_width && newH == m_height && m_hizTexture) return;

    ReleaseResources();
    m_width = newW;
    m_height = newH;
    UINT maxDim = std::max(m_width, m_height);
    m_mipLevels = (UINT)std::floor(std::log2((float)maxDim)) + 1;
    CreateResources(device);
}

void HiZManager::ReleaseResources() {
    if (m_hizTexture) { m_hizTexture->Release(); m_hizTexture = nullptr; }
    if (m_descriptorHeap) { m_descriptorHeap->Release(); m_descriptorHeap = nullptr; }
    m_uavCpuHandles.clear();
    m_uavGpuHandles.clear();
    m_srvMipCpuHandles.clear();
    m_srvMipGpuHandles.clear();
}

bool HiZManager::CreateResources(ID3D12Device* device) {
    if (!device || m_width == 0 || m_height == 0) return false;

    // Create Hi-Z Texture2D with full mip chain
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = m_width;
    texDesc.Height = m_height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = (UINT16)m_mipLevels;
    texDesc.Format = DXGI_FORMAT_R32_FLOAT;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        nullptr,
        IID_PPV_ARGS(&m_hizTexture)
    );
    if (FAILED(hr)) {
        std::cerr << "[MeshClusterCulling] Failed to create Hi-Z texture: " << hr << std::endl;
        return false;
    }
    m_hizTexture->SetName(L"HiZ_DepthPyramid");

    // Allocate Descriptor Heap:
    // 1 Full SRV + m_mipLevels UAVs + m_mipLevels per-mip SRVs
    UINT totalDescriptors = 1 + m_mipLevels + m_mipLevels;
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = totalDescriptors;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_descriptorHeap));
    if (FAILED(hr)) {
        std::cerr << "[MeshClusterCulling] Failed to create Hi-Z descriptor heap: " << hr << std::endl;
        return false;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE cpuStart = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE gpuStart = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();

    // 1. Full Pyramid SRV (All Mips)
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = m_mipLevels;
    device->CreateShaderResourceView(m_hizTexture, &srvDesc, cpuStart);

    m_srvGpuHandle = gpuStart;

    // Offset past the full SRV
    cpuStart.ptr += m_descriptorSize;
    gpuStart.ptr += m_descriptorSize;

    // 2. Per-Mip UAVs
    m_uavCpuHandles.resize(m_mipLevels);
    m_uavGpuHandles.resize(m_mipLevels);
    for (UINT m = 0; m < m_mipLevels; ++m) {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = m;

        device->CreateUnorderedAccessView(m_hizTexture, nullptr, &uavDesc, cpuStart);
        m_uavCpuHandles[m] = cpuStart;
        m_uavGpuHandles[m] = gpuStart;

        cpuStart.ptr += m_descriptorSize;
        gpuStart.ptr += m_descriptorSize;
    }

    // 3. Per-Mip SRVs (Single Mip)
    m_srvMipCpuHandles.resize(m_mipLevels);
    m_srvMipGpuHandles.resize(m_mipLevels);
    for (UINT m = 0; m < m_mipLevels; ++m) {
        D3D12_SHADER_RESOURCE_VIEW_DESC mipSrvDesc = {};
        mipSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        mipSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        mipSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        mipSrvDesc.Texture2D.MostDetailedMip = m;
        mipSrvDesc.Texture2D.MipLevels = 1;

        device->CreateShaderResourceView(m_hizTexture, &mipSrvDesc, cpuStart);
        m_srvMipCpuHandles[m] = cpuStart;
        m_srvMipGpuHandles[m] = gpuStart;

        cpuStart.ptr += m_descriptorSize;
        gpuStart.ptr += m_descriptorSize;
    }

    return true;
}

bool HiZManager::CreatePipelines(ID3D12Device* device) {
    // Root Signature for HiZ downsampling compute shader:
    // Slot 0: Root Constants (b0) - 6 uints (srcTexelSize, dstTexelSize, dstMipSize, isFirstMip)
    // Slot 1: Descriptor Table (t0) - SRV (source mip)
    // Slot 2: Descriptor Table (u0) - UAV (destination mip)

    D3D12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 1;
    srvRange.BaseShaderRegister = 0;
    srvRange.RegisterSpace = 0;
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE uavRange = {};
    uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange.NumDescriptors = 1;
    uavRange.BaseShaderRegister = 0;
    uavRange.RegisterSpace = 0;
    uavRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParams[3] = {};

    // 0: Constants
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParams[0].Constants.ShaderRegister = 0;
    rootParams[0].Constants.RegisterSpace = 0;
    rootParams[0].Constants.Num32BitValues = 8;

    // 1: SRV
    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[1].DescriptorTable.pDescriptorRanges = &srvRange;

    // 2: UAV
    rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[2].DescriptorTable.pDescriptorRanges = &uavRange;

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.MipLODBias = 0;
    sampler.MaxAnisotropy = 0;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 3;
    rootSigDesc.pParameters = rootParams;
    rootSigDesc.NumStaticSamplers = 1;
    rootSigDesc.pStaticSamplers = &sampler;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ID3DBlob* signatureBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            std::cerr << "[MeshClusterCulling] HiZ RootSig serialization error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
            errorBlob->Release();
        }
        return false;
    }

    hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
    signatureBlob->Release();
    if (FAILED(hr)) return false;

    // Compile HiZBuild.hlsl
    ID3DBlob* csBlob = nullptr;
    ID3DBlob* csErrors = nullptr;
    const char* shaderPaths[] = {
        "Plugins/Mesh-Cluster-Culling/Shaders/HiZBuild.hlsl",
        "Shaders/HiZBuild.hlsl",
        "../Plugins/Mesh-Cluster-Culling/Shaders/HiZBuild.hlsl"
    };

    std::wstring chosenPath;
    for (const char* p : shaderPaths) {
        std::wstring wp(p, p + strlen(p));
        hr = D3DCompileFromFile(wp.c_str(), nullptr, nullptr, "CSMain", "cs_5_1", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &csBlob, &csErrors);
        if (SUCCEEDED(hr)) {
            chosenPath = wp;
            break;
        }
    }

    if (FAILED(hr)) {
        if (csErrors) {
            std::cerr << "[MeshClusterCulling] Failed to compile HiZBuild.hlsl: " << (char*)csErrors->GetBufferPointer() << std::endl;
            csErrors->Release();
        }
        return false;
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_rootSignature;
    psoDesc.CS.pShaderBytecode = csBlob->GetBufferPointer();
    psoDesc.CS.BytecodeLength = csBlob->GetBufferSize();

    hr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_hizBuildPSO));
    csBlob->Release();
    return SUCCEEDED(hr);
}

void HiZManager::GeneratePyramid(
    ID3D12GraphicsCommandList* cmdList,
    ID3D12Resource* sceneDepthBuffer,
    D3D12_RESOURCE_STATES currentDepthState)
{
    if (!cmdList || !m_isReady || !m_hizTexture || m_mipLevels == 0) return;

    // Transition Hi-Z texture to UAV for compute writing
    D3D12_RESOURCE_BARRIER toUav = {};
    toUav.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toUav.Transition.pResource = m_hizTexture;
    toUav.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    toUav.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    toUav.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    cmdList->ResourceBarrier(1, &toUav);

    ID3D12DescriptorHeap* heaps[] = { m_descriptorHeap };
    cmdList->SetDescriptorHeaps(1, heaps);
    cmdList->SetComputeRootSignature(m_rootSignature);
    cmdList->SetPipelineState(m_hizBuildPSO);

    // Mip 0 downsample/copy from base depth or initialize
    // Downsample loop across all mips
    for (UINT m = 0; m < m_mipLevels; ++m) {
        UINT dstW = std::max(1u, m_width >> m);
        UINT dstH = std::max(1u, m_height >> m);
        UINT srcW = (m == 0) ? m_width : std::max(1u, m_width >> (m - 1));
        UINT srcH = (m == 0) ? m_height : std::max(1u, m_height >> (m - 1));

        struct {
            float srcTexelSize[2];
            float dstTexelSize[2];
            UINT  dstMipSize[2];
            UINT  isFirstMip;
            UINT  pad;
        } cbData;

        cbData.srcTexelSize[0] = 1.0f / (float)srcW;
        cbData.srcTexelSize[1] = 1.0f / (float)srcH;
        cbData.dstTexelSize[0] = 1.0f / (float)dstW;
        cbData.dstTexelSize[1] = 1.0f / (float)dstH;
        cbData.dstMipSize[0] = dstW;
        cbData.dstMipSize[1] = dstH;
        cbData.isFirstMip = (m == 0) ? 1 : 0;
        cbData.pad = 0;

        cmdList->SetComputeRoot32BitConstants(0, 8, &cbData, 0);

        // Bind source mip SRV: for m > 0 use m-1 mip SRV; for m == 0 use default fallback or prior mip
        UINT srcMipIdx = (m == 0) ? 0 : (m - 1);
        cmdList->SetComputeRootDescriptorTable(1, m_srvMipGpuHandles[srcMipIdx]);
        cmdList->SetComputeRootDescriptorTable(2, m_uavGpuHandles[m]);

        UINT groupsX = (dstW + 7) / 8;
        UINT groupsY = (dstH + 7) / 8;
        cmdList->Dispatch(groupsX, groupsY, 1);

        // UAV barrier between consecutive mip levels to ensure completion
        D3D12_RESOURCE_BARRIER uavBarrier = {};
        uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        uavBarrier.UAV.pResource = m_hizTexture;
        cmdList->ResourceBarrier(1, &uavBarrier);
    }

    // Transition Hi-Z texture back to shader resource for cluster culling pass
    D3D12_RESOURCE_BARRIER toSrv = {};
    toSrv.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toSrv.Transition.pResource = m_hizTexture;
    toSrv.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    toSrv.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    toSrv.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    cmdList->ResourceBarrier(1, &toSrv);
}

} // namespace Eunoia
