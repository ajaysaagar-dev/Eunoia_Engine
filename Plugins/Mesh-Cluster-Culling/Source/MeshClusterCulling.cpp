// ============================================================================
// Eunoia Engine — Mesh Cluster Culling Plugin
// MeshClusterCulling: Core GPU culling system implementation & plugin declaration
// ============================================================================

#include "MeshClusterCulling/MeshClusterCulling.h"
#define EUNOIA_PLUGIN_STATIC
#include <EunoiaPluginCore/IPlugin.h>
#include <EunoiaPluginCore/PluginAPI.h>
#include <EunoiaPluginCore/ServiceRegistry.h>
#include <d3dcompiler.h>
#include <iostream>
#include <algorithm>
#include <chrono>

namespace Eunoia {

MeshClusterCullingSystem& MeshClusterCullingSystem::Get() {
    static MeshClusterCullingSystem s_instance;
    return s_instance;
}

MeshClusterCullingSystem::MeshClusterCullingSystem() = default;

MeshClusterCullingSystem::~MeshClusterCullingSystem() {
    Shutdown();
}

bool MeshClusterCullingSystem::Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue, UINT width, UINT height) {
    if (!device) return false;
    m_device = device;
    m_commandQueue = commandQueue;

    std::cout << "[MeshClusterCulling] Initializing DirectX 12 GPU Mesh Cluster Culling Pipeline..." << std::endl;

    m_descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    if (!m_hizManager.Initialize(device, width, height)) {
        std::cerr << "[MeshClusterCulling] Warning: Hi-Z initialization failed; falling back to frustum/cone culling only." << std::endl;
        m_config.enableHiZOcclusion = false;
    }

    if (!CreateRootSignaturesAndPipelines()) {
        std::cerr << "[MeshClusterCulling] Failed to create culling root signature or pipeline state." << std::endl;
        return false;
    }

    if (!CreateBuffers()) {
        std::cerr << "[MeshClusterCulling] Failed to allocate GPU cluster buffers." << std::endl;
        return false;
    }

    m_isInitialized = true;
    std::cout << "[MeshClusterCulling] Plugin successfully initialized and registered with engine." << std::endl;
    return true;
}

void MeshClusterCullingSystem::Shutdown() {
    ReleaseBuffers();
    m_hizManager.Shutdown();

    if (m_indirectCommandSignature) { m_indirectCommandSignature->Release(); m_indirectCommandSignature = nullptr; }
    if (m_cullPSO) { m_cullPSO->Release(); m_cullPSO = nullptr; }
    if (m_cullRootSignature) { m_cullRootSignature->Release(); m_cullRootSignature = nullptr; }

    m_isInitialized = false;
    std::cout << "[MeshClusterCulling] Plugin shutdown cleanly." << std::endl;
}

void MeshClusterCullingSystem::Resize(UINT width, UINT height) {
    if (!m_isInitialized) return;
    m_hizManager.Resize(m_device, width, height);
}

bool MeshClusterCullingSystem::CreateRootSignaturesAndPipelines() {
    // Root Signature for ClusterCull.hlsl:
    // Slot 0: CBV (CameraCullConstants) - b0
    // Slot 1: Descriptor Table (t0: Clusters SRV, t1: Hi-Z Pyramid SRV)
    // Slot 2: Descriptor Table (u0: VisibleIds, u1: IndirectArgs, u2: Counters)

    D3D12_DESCRIPTOR_RANGE srvRange = {};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 2; // t0, t1
    srvRange.BaseShaderRegister = 0;
    srvRange.RegisterSpace = 0;
    srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_DESCRIPTOR_RANGE uavRange = {};
    uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    uavRange.NumDescriptors = 3; // u0, u1, u2
    uavRange.BaseShaderRegister = 0;
    uavRange.RegisterSpace = 0;
    uavRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParams[3] = {};

    // 0: CBV (CameraCullConstants)
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParams[0].Descriptor.ShaderRegister = 0;
    rootParams[0].Descriptor.RegisterSpace = 0;

    // 1: SRV table (Clusters, HiZ)
    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[1].DescriptorTable.pDescriptorRanges = &srvRange;

    // 2: UAV table (VisibleIds, IndirectArgs, Counters)
    rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
    rootParams[2].DescriptorTable.pDescriptorRanges = &uavRange;

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
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

    ID3DBlob* sigBlob = nullptr;
    ID3DBlob* errBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
    if (FAILED(hr)) {
        if (errBlob) {
            std::cerr << "[MeshClusterCulling] RootSig serialization error: " << (char*)errBlob->GetBufferPointer() << std::endl;
            errBlob->Release();
        }
        return false;
    }

    hr = m_device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&m_cullRootSignature));
    sigBlob->Release();
    if (FAILED(hr)) return false;

    // Compile ClusterCull.hlsl
    ID3DBlob* csBlob = nullptr;
    ID3DBlob* csErrors = nullptr;
    const char* shaderPaths[] = {
        "Plugins/Mesh-Cluster-Culling/Shaders/ClusterCull.hlsl",
        "Shaders/ClusterCull.hlsl",
        "../Plugins/Mesh-Cluster-Culling/Shaders/ClusterCull.hlsl"
    };

    for (const char* p : shaderPaths) {
        std::wstring wp(p, p + strlen(p));
        hr = D3DCompileFromFile(wp.c_str(), nullptr, nullptr, "CSMain", "cs_5_1", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &csBlob, &csErrors);
        if (SUCCEEDED(hr)) break;
    }

    if (FAILED(hr)) {
        if (csErrors) {
            std::cerr << "[MeshClusterCulling] Failed to compile ClusterCull.hlsl: " << (char*)csErrors->GetBufferPointer() << std::endl;
            csErrors->Release();
        }
        return false;
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_cullRootSignature;
    psoDesc.CS.pShaderBytecode = csBlob->GetBufferPointer();
    psoDesc.CS.BytecodeLength = csBlob->GetBufferSize();

    hr = m_device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_cullPSO));
    csBlob->Release();
    if (FAILED(hr)) return false;

    // Create Indirect Command Signature for DrawIndexedInstanced
    D3D12_INDIRECT_ARGUMENT_DESC argDesc = {};
    argDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

    D3D12_COMMAND_SIGNATURE_DESC cmdSigDesc = {};
    cmdSigDesc.ByteStride = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
    cmdSigDesc.NumArgumentDescs = 1;
    cmdSigDesc.pArgumentDescs = &argDesc;
    cmdSigDesc.NodeMask = 0;

    hr = m_device->CreateCommandSignature(&cmdSigDesc, nullptr, IID_PPV_ARGS(&m_indirectCommandSignature));
    return SUCCEEDED(hr);
}

bool MeshClusterCullingSystem::CreateBuffers() {
    D3D12_HEAP_PROPERTIES defaultHeap = { D3D12_HEAP_TYPE_DEFAULT };
    D3D12_HEAP_PROPERTIES uploadHeap = { D3D12_HEAP_TYPE_UPLOAD };
    D3D12_HEAP_PROPERTIES readbackHeap = { D3D12_HEAP_TYPE_READBACK };

    UINT64 clusterBufSize = (UINT64)m_maxClusters * sizeof(GPUClusterData);
    UINT64 indirectArgsSize = (UINT64)m_maxClusters * sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
    UINT64 visibleIdsSize = (UINT64)m_maxClusters * sizeof(uint32_t);
    UINT64 counterSize = 8 * sizeof(uint32_t); // visibleCount, culledCount + padding

    // 1. Cluster data buffer (Default heap + UAV/SRV)
    D3D12_RESOURCE_DESC clusterDesc = {};
    clusterDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    clusterDesc.Width = clusterBufSize;
    clusterDesc.Height = 1;
    clusterDesc.DepthOrArraySize = 1;
    clusterDesc.MipLevels = 1;
    clusterDesc.SampleDesc.Count = 1;
    clusterDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    clusterDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HRESULT hr = m_device->CreateCommittedResource(
        &defaultHeap, D3D12_HEAP_FLAG_NONE, &clusterDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
        IID_PPV_ARGS(&m_clusterDataBuffer)
    );
    if (FAILED(hr)) return false;

    // Cluster upload buffer
    hr = m_device->CreateCommittedResource(
        &uploadHeap, D3D12_HEAP_FLAG_NONE, &clusterDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&m_clusterDataUploadBuffer)
    );
    if (FAILED(hr)) return false;

    // 2. Visible IDs Buffer
    D3D12_RESOURCE_DESC visibleDesc = clusterDesc;
    visibleDesc.Width = visibleIdsSize;
    visibleDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    hr = m_device->CreateCommittedResource(
        &defaultHeap, D3D12_HEAP_FLAG_NONE, &visibleDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
        IID_PPV_ARGS(&m_visibleIdsBuffer)
    );
    if (FAILED(hr)) return false;

    // 3. Indirect Draw Arguments Buffer
    D3D12_RESOURCE_DESC indirectDesc = clusterDesc;
    indirectDesc.Width = indirectArgsSize;
    indirectDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    hr = m_device->CreateCommittedResource(
        &defaultHeap, D3D12_HEAP_FLAG_NONE, &indirectDesc,
        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, nullptr,
        IID_PPV_ARGS(&m_indirectArgsBuffer)
    );
    if (FAILED(hr)) return false;

    // 4. Counter buffer (UAV for atomic adds)
    D3D12_RESOURCE_DESC counterDesc = clusterDesc;
    counterDesc.Width = counterSize;
    counterDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    hr = m_device->CreateCommittedResource(
        &defaultHeap, D3D12_HEAP_FLAG_NONE, &counterDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
        IID_PPV_ARGS(&m_counterBuffer)
    );
    if (FAILED(hr)) return false;

    // 5. Readback counter buffer
    D3D12_RESOURCE_DESC readbackDesc = clusterDesc;
    readbackDesc.Width = counterSize;
    readbackDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    hr = m_device->CreateCommittedResource(
        &readbackHeap, D3D12_HEAP_FLAG_NONE, &readbackDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
        IID_PPV_ARGS(&m_readbackCounterBuffer)
    );
    if (FAILED(hr)) return false;

    // 6. Camera Constant Buffer (256-byte aligned)
    D3D12_RESOURCE_DESC cbDesc = clusterDesc;
    cbDesc.Width = 256;
    cbDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    hr = m_device->CreateCommittedResource(
        &uploadHeap, D3D12_HEAP_FLAG_NONE, &cbDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&m_constantBuffer)
    );
    if (FAILED(hr)) return false;

    // 7. Descriptor Heap for Culling (2 SRVs + 3 UAVs = 5 descriptors)
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 5;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    hr = m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_cullDescriptorHeap));
    if (FAILED(hr)) return false;

    D3D12_CPU_DESCRIPTOR_HANDLE cpuStart = m_cullDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    // Descriptor 0: t0 - StructuredBuffer<GPUClusterData>
    D3D12_SHADER_RESOURCE_VIEW_DESC clusterSrv = {};
    clusterSrv.Format = DXGI_FORMAT_UNKNOWN;
    clusterSrv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    clusterSrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    clusterSrv.Buffer.FirstElement = 0;
    clusterSrv.Buffer.NumElements = m_maxClusters;
    clusterSrv.Buffer.StructureByteStride = sizeof(GPUClusterData);
    clusterSrv.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    m_device->CreateShaderResourceView(m_clusterDataBuffer, &clusterSrv, cpuStart);

    // Descriptor 1: t1 - Texture2D Hi-Z (placed from Hi-Z manager or bound per frame)
    cpuStart.ptr += m_descriptorSize;
    if (m_hizManager.GetHiZTexture()) {
        D3D12_SHADER_RESOURCE_VIEW_DESC hizSrv = {};
        hizSrv.Format = DXGI_FORMAT_R32_FLOAT;
        hizSrv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        hizSrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        hizSrv.Texture2D.MostDetailedMip = 0;
        hizSrv.Texture2D.MipLevels = m_hizManager.GetMipLevels();
        m_device->CreateShaderResourceView(m_hizManager.GetHiZTexture(), &hizSrv, cpuStart);
    }

    // Descriptor 2: u0 - RWStructuredBuffer<uint> VisibleClusterIds
    cpuStart.ptr += m_descriptorSize;
    D3D12_UNORDERED_ACCESS_VIEW_DESC visUav = {};
    visUav.Format = DXGI_FORMAT_UNKNOWN;
    visUav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    visUav.Buffer.FirstElement = 0;
    visUav.Buffer.NumElements = m_maxClusters;
    visUav.Buffer.StructureByteStride = sizeof(uint32_t);
    m_device->CreateUnorderedAccessView(m_visibleIdsBuffer, nullptr, &visUav, cpuStart);

    // Descriptor 3: u1 - RWStructuredBuffer<DrawIndexedArguments> IndirectArgs
    cpuStart.ptr += m_descriptorSize;
    D3D12_UNORDERED_ACCESS_VIEW_DESC argsUav = {};
    argsUav.Format = DXGI_FORMAT_UNKNOWN;
    argsUav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    argsUav.Buffer.FirstElement = 0;
    argsUav.Buffer.NumElements = m_maxClusters;
    argsUav.Buffer.StructureByteStride = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
    m_device->CreateUnorderedAccessView(m_indirectArgsBuffer, nullptr, &argsUav, cpuStart);

    // Descriptor 4: u2 - RWStructuredBuffer<uint> Counters
    cpuStart.ptr += m_descriptorSize;
    D3D12_UNORDERED_ACCESS_VIEW_DESC counterUav = {};
    counterUav.Format = DXGI_FORMAT_UNKNOWN;
    counterUav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    counterUav.Buffer.FirstElement = 0;
    counterUav.Buffer.NumElements = 8;
    counterUav.Buffer.StructureByteStride = sizeof(uint32_t);
    m_device->CreateUnorderedAccessView(m_counterBuffer, nullptr, &counterUav, cpuStart);

    return true;
}

void MeshClusterCullingSystem::ReleaseBuffers() {
    if (m_cullDescriptorHeap) { m_cullDescriptorHeap->Release(); m_cullDescriptorHeap = nullptr; }
    if (m_constantBuffer) { m_constantBuffer->Release(); m_constantBuffer = nullptr; }
    if (m_readbackCounterBuffer) { m_readbackCounterBuffer->Release(); m_readbackCounterBuffer = nullptr; }
    if (m_counterBuffer) { m_counterBuffer->Release(); m_counterBuffer = nullptr; }
    if (m_indirectArgsBuffer) { m_indirectArgsBuffer->Release(); m_indirectArgsBuffer = nullptr; }
    if (m_visibleIdsBuffer) { m_visibleIdsBuffer->Release(); m_visibleIdsBuffer = nullptr; }
    if (m_clusterDataUploadBuffer) { m_clusterDataUploadBuffer->Release(); m_clusterDataUploadBuffer = nullptr; }
    if (m_clusterDataBuffer) { m_clusterDataBuffer->Release(); m_clusterDataBuffer = nullptr; }
}

void MeshClusterCullingSystem::UpdateCamera(
    const glm::mat4& viewMatrix,
    const glm::mat4& projMatrix,
    const glm::vec3& cameraPos,
    const glm::vec4 frustumPlanes[6],
    float viewportWidth,
    float viewportHeight)
{
    m_cameraConstants.viewProj = projMatrix * viewMatrix;
    for (int i = 0; i < 6; ++i) {
        m_cameraConstants.frustumPlanes[i] = frustumPlanes[i];
    }
    m_cameraConstants.cameraPos = cameraPos;
    m_cameraConstants.viewportSize = glm::vec2(viewportWidth, viewportHeight);
    m_cameraConstants.minScreenSize = m_config.minScreenSizePixels;
    m_cameraConstants.enableFrustum = m_config.enableFrustumCulling ? 1 : 0;
    m_cameraConstants.enableBackface = m_config.enableBackfaceCulling ? 1 : 0;
    m_cameraConstants.enableHiZ = m_config.enableHiZOcclusion ? 1 : 0;
    m_cameraConstants.enableScreenSize = m_config.enableScreenSizeCulling ? 1 : 0;
    m_cameraConstants.debugMode = (uint32_t)m_config.debugVisualizationMode;

    if (m_hizManager.IsReady()) {
        m_cameraConstants.hizSize = glm::vec2((float)m_hizManager.GetWidth(), (float)m_hizManager.GetHeight());
        m_cameraConstants.hizMaxMip = (float)(m_hizManager.GetMipLevels() - 1);
    } else {
        m_cameraConstants.enableHiZ = 0;
        m_cameraConstants.hizMaxMip = 0.0f;
    }
}

bool MeshClusterCullingSystem::EnsureClustersBuilt(GameObject& obj) {
    if (!obj.meshClusterCulling || obj.mesh.vertices.empty() || obj.mesh.indices.empty()) {
        return false;
    }

    std::string key = obj.meshFilePath.empty() ? ("obj_" + std::to_string(obj.id)) : obj.meshFilePath;
    ClusteredMesh clustered = MeshClusterBuilder::Get().BuildOrGetClusters(obj.mesh, key, m_config);
    return clustered.isValid;
}

bool MeshClusterCullingSystem::ExecuteCullingPass(
    ID3D12GraphicsCommandList* cmdList,
    Scene& scene,
    ID3D12Resource* sceneDepthBuffer,
    D3D12_RESOURCE_STATES currentDepthState)
{
    if (!m_isInitialized || !m_config.enabled || !cmdList) {
        return false;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    m_objectDrawRanges.clear();
    std::vector<GPUClusterData> gpuClusters;
    uint32_t totalClusters = 0;
    uint32_t objectsEnabledCount = 0;

    // Collect all enabled objects
    for (const auto& obj : scene.objects) {
        if (!obj.meshClusterCulling || !obj.visible || obj.isCamera || obj.isLight) continue;
        if (obj.mesh.vertices.empty() || obj.mesh.indices.empty()) continue;

        std::string key = obj.meshFilePath.empty() ? ("obj_" + std::to_string(obj.id)) : obj.meshFilePath;
        ClusteredMesh clustered = MeshClusterBuilder::Get().BuildOrGetClusters(obj.mesh, key, m_config);
        if (!clustered.isValid || clustered.clusters.empty()) continue;

        objectsEnabledCount++;
        glm::mat4 worldMatrix = scene.GetWorldMatrix(obj);

        ObjectClusterDrawRange range;
        range.firstClusterIndex = (uint32_t)gpuClusters.size();
        range.clusterCount = (uint32_t)clustered.clusters.size();
        range.visibleCount = range.clusterCount;

        for (const auto& c : clustered.clusters) {
            GPUClusterData gd = {};
            gd.boundsMin = c.boundsMin;
            gd.boundsMax = c.boundsMax;
            gd.sphereCenter = c.sphereCenter;
            gd.sphereRadius = c.sphereRadius;
            gd.coneApex = c.coneApex;
            gd.coneAxis = c.coneAxis;
            gd.coneCutoff = c.coneCutoff;
            gd.indexOffset = c.indexOffset;
            gd.indexCount = c.indexCount;
            gd.baseVertex = c.vertexOffset;
            gd.worldMatrix = worldMatrix;

            gpuClusters.push_back(gd);
            if (gpuClusters.size() >= m_maxClusters) break;
        }

        m_objectDrawRanges[obj.id] = range;
        if (gpuClusters.size() >= m_maxClusters) break;
    }

    totalClusters = (uint32_t)gpuClusters.size();
    m_currentClusterCount = totalClusters;
    m_stats.objectsEnabled = objectsEnabledCount;
    m_stats.totalClusters = totalClusters;

    if (totalClusters == 0) {
        m_stats.visibleClusters = 0;
        m_stats.culledClusters = 0;
        m_stats.Finalize();
        return false;
    }

    // 1. Upload cluster data
    void* pClusterMap = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    if (SUCCEEDED(m_clusterDataUploadBuffer->Map(0, &readRange, &pClusterMap))) {
        memcpy(pClusterMap, gpuClusters.data(), totalClusters * sizeof(GPUClusterData));
        m_clusterDataUploadBuffer->Unmap(0, nullptr);
    }

    // Transition cluster buffer to COPY_DEST
    D3D12_RESOURCE_BARRIER copyBarriers[1] = {};
    copyBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    copyBarriers[0].Transition.pResource = m_clusterDataBuffer;
    copyBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    copyBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    cmdList->ResourceBarrier(1, copyBarriers);

    cmdList->CopyBufferRegion(m_clusterDataBuffer, 0, m_clusterDataUploadBuffer, 0, totalClusters * sizeof(GPUClusterData));

    copyBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    copyBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    cmdList->ResourceBarrier(1, copyBarriers);

    // 2. Upload Camera Constants
    m_cameraConstants.totalClusters = totalClusters;
    void* pCbMap = nullptr;
    if (SUCCEEDED(m_constantBuffer->Map(0, &readRange, &pCbMap))) {
        memcpy(pCbMap, &m_cameraConstants, sizeof(CameraCullConstants));
        m_constantBuffer->Unmap(0, nullptr);
    }

    // 3. Generate Hi-Z depth pyramid if enabled
    if (m_config.enableHiZOcclusion && sceneDepthBuffer && m_hizManager.IsReady()) {
        m_hizManager.GeneratePyramid(cmdList, sceneDepthBuffer, currentDepthState);
    }

    // 4. Transition buffers for compute writing
    D3D12_RESOURCE_BARRIER uavBarriers[2] = {};
    uavBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    uavBarriers[0].Transition.pResource = m_indirectArgsBuffer;
    uavBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    uavBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    uavBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    uavBarriers[1].Transition.pResource = m_counterBuffer;
    uavBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    uavBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    cmdList->ResourceBarrier(2, uavBarriers);

    // 5. Dispatch Compute Culling Shader
    ID3D12DescriptorHeap* heaps[] = { m_cullDescriptorHeap };
    cmdList->SetDescriptorHeaps(1, heaps);
    cmdList->SetComputeRootSignature(m_cullRootSignature);
    cmdList->SetPipelineState(m_cullPSO);

    cmdList->SetComputeRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());

    D3D12_GPU_DESCRIPTOR_HANDLE gpuHeapStart = m_cullDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE srvTable = gpuHeapStart;
    D3D12_GPU_DESCRIPTOR_HANDLE uavTable = gpuHeapStart;
    uavTable.ptr += 2 * m_descriptorSize; // Offset past 2 SRVs

    cmdList->SetComputeRootDescriptorTable(1, srvTable);
    cmdList->SetComputeRootDescriptorTable(2, uavTable);

    UINT threadGroups = (totalClusters + 63) / 64;
    cmdList->Dispatch(threadGroups, 1, 1);

    // 6. Transition indirect arguments buffer to INDIRECT_ARGUMENT
    D3D12_RESOURCE_BARRIER postBarriers[2] = {};
    postBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    postBarriers[0].Transition.pResource = m_indirectArgsBuffer;
    postBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    postBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;

    postBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    postBarriers[1].Transition.pResource = m_counterBuffer;
    postBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    postBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    cmdList->ResourceBarrier(2, postBarriers);

    // 7. Copy counters to readback buffer for non-stalling CPU statistics
    cmdList->CopyBufferRegion(m_readbackCounterBuffer, 0, m_counterBuffer, 0, 8 * sizeof(uint32_t));

    auto endTime = std::chrono::high_resolution_clock::now();
    m_stats.cpuCullingTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();

    return true;
}

bool MeshClusterCullingSystem::RenderClusteredBatch(
    ID3D12GraphicsCommandList* cmdList,
    const Scene::RenderBatch& batch)
{
    if (!m_isInitialized || !m_config.enabled || !cmdList || !m_indirectCommandSignature) {
        return false;
    }

    auto it = m_objectDrawRanges.find(batch.objectId);
    if (it == m_objectDrawRanges.end()) {
        return false; // Fallback to normal draw
    }

    const auto& range = it->second;
    if (range.clusterCount == 0) return false;

    // Byte offset into indirect argument buffer
    UINT64 byteOffset = (UINT64)range.firstClusterIndex * sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);

    cmdList->ExecuteIndirect(
        m_indirectCommandSignature,
        range.clusterCount,
        m_indirectArgsBuffer,
        byteOffset,
        nullptr,
        0
    );

    m_stats.indirectDraws++;
    return true;
}

void MeshClusterCullingSystem::FetchStatistics() {
    if (!m_isInitialized || !m_readbackCounterBuffer) return;

    D3D12_RANGE readRange = { 0, 8 * sizeof(uint32_t) };
    uint32_t* pCounters = nullptr;
    if (SUCCEEDED(m_readbackCounterBuffer->Map(0, &readRange, (void**)&pCounters))) {
        if (pCounters) {
            uint32_t vis = pCounters[0];
            uint32_t culled = pCounters[1];

            // Only update if reasonable
            if (vis + culled > 0 && vis + culled <= m_maxClusters * 2) {
                m_lastRecordedVisible = vis;
                m_lastRecordedCulled = culled;
            }
        }
        m_readbackCounterBuffer->Unmap(0, nullptr);
    }

    m_stats.visibleClusters = m_lastRecordedVisible;
    m_stats.culledClusters = m_lastRecordedCulled;
    m_stats.Finalize();
}

ObjectClusterStats MeshClusterCullingSystem::GetObjectStats(int objectId) const {
    ObjectClusterStats res = {};
    auto it = m_objectDrawRanges.find(objectId);
    if (it != m_objectDrawRanges.end()) {
        res.enabled = true;
        res.totalClusters = it->second.clusterCount;
        // Estimated visible based on global ratio or range
        float ratio = m_stats.totalClusters > 0 ? ((float)m_stats.visibleClusters / (float)m_stats.totalClusters) : 1.0f;
        res.visibleClusters = (uint32_t)std::round(res.totalClusters * ratio);
        res.culledClusters = res.totalClusters - res.visibleClusters;
        res.cullingRatio = res.totalClusters > 0 ? ((float)res.culledClusters / (float)res.totalClusters * 100.0f) : 0.0f;
    }
    return res;
}

} // namespace Eunoia

// ============================================================================
// Eunoia Engine Plugin Export
// ============================================================================

class MeshClusterCullingPlugin : public IPlugin {
public:
    explicit MeshClusterCullingPlugin(const EunoiaPluginContext* /*ctx*/) {}
    ~MeshClusterCullingPlugin() override = default;

    const char* GetName() const override { return "Mesh-Cluster-Culling"; }
    bool SupportsHotReload() const override { return false; }

    void OnRegister(ServiceRegistry& registry) override {
        std::cout << "[MeshClusterCulling] Registering Mesh-Cluster-Culling services..." << std::endl;
    }

    void OnInit() override {
        std::cout << "[MeshClusterCulling] Plugin initialized." << std::endl;
    }

    void OnShutdown() override {
        Eunoia::MeshClusterCullingSystem::Get().Shutdown();
        std::cout << "[MeshClusterCulling] Plugin shutdown." << std::endl;
    }
};

static EunoiaPluginInfo s_clusterCullingInfo = {
    "Mesh-Cluster-Culling",
    "1.0.0",
    "DirectX 12 GPU-driven meshlet and cluster visibility culling pipeline with Hi-Z",
    "Eunoia Team",
    EUNOIA_PLUGIN_API_VERSION,
    EunoiaPluginCategory::Rendering,
    false // dynamic or core tier
};

EUNOIA_DECLARE_PLUGIN(MeshClusterCullingPlugin, &s_clusterCullingInfo)
