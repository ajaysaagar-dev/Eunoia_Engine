#pragma once
// ============================================================================
// Eunoia Engine — Mesh Cluster Culling Plugin
// MeshClusterCulling: Core GPU culling system and plugin interface
// ============================================================================

#include "MeshClusterTypes.h"
#include "MeshClusterBuilder.h"
#include "HiZManager.h"
#include <EngineScene/Scene.h>
#include <EngineRenderer/Camera.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <chrono>

namespace Eunoia {

class MeshClusterCullingSystem {
public:
    static MeshClusterCullingSystem& Get();

    bool Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue, UINT width = 1280, UINT height = 720);
    void Shutdown();
    void Resize(UINT width, UINT height);

    // Frame update for camera & scene data
    void UpdateCamera(
        const glm::mat4& viewMatrix,
        const glm::mat4& projMatrix,
        const glm::vec3& cameraPos,
        const glm::vec4 frustumPlanes[6],
        float viewportWidth,
        float viewportHeight
    );

    // Build or update clusters for a GameObject
    bool EnsureClustersBuilt(GameObject& obj);

    // Execute GPU Culling pass for all enabled objects in the scene
    bool ExecuteCullingPass(
        ID3D12GraphicsCommandList* cmdList,
        Scene& scene,
        ID3D12Resource* sceneDepthBuffer,
        D3D12_RESOURCE_STATES currentDepthState
    );

    // Execute indirect draws for a specific batch/object if clustered
    bool RenderClusteredBatch(
        ID3D12GraphicsCommandList* cmdList,
        const Scene::RenderBatch& batch
    );

    // Read back GPU visibility statistics (non-stalling)
    void FetchStatistics();

    // Configuration & Inspection
    ClusterCullingConfig& GetConfig() { return m_config; }
    const ClusterCullingConfig& GetConfig() const { return m_config; }
    void SetConfig(const ClusterCullingConfig& cfg) { m_config = cfg; }

    const ClusterCullingStats& GetStats() const { return m_stats; }
    ObjectClusterStats GetObjectStats(int objectId) const;

    HiZManager& GetHiZManager() { return m_hizManager; }
    bool IsInitialized() const { return m_isInitialized; }

private:
    MeshClusterCullingSystem();
    ~MeshClusterCullingSystem();

    bool CreateRootSignaturesAndPipelines();
    bool CreateBuffers();
    void ReleaseBuffers();

    ID3D12Device*             m_device = nullptr;
    ID3D12CommandQueue*       m_commandQueue = nullptr;
    ID3D12RootSignature*      m_cullRootSignature = nullptr;
    ID3D12PipelineState*      m_cullPSO = nullptr;
    ID3D12CommandSignature*   m_indirectCommandSignature = nullptr;

    // GPU Resource Buffers
    ID3D12Resource*           m_clusterDataBuffer = nullptr;
    ID3D12Resource*           m_clusterDataUploadBuffer = nullptr;
    ID3D12Resource*           m_visibleIdsBuffer = nullptr;
    ID3D12Resource*           m_indirectArgsBuffer = nullptr;
    ID3D12Resource*           m_counterBuffer = nullptr;
    ID3D12Resource*           m_readbackCounterBuffer = nullptr;
    ID3D12Resource*           m_constantBuffer = nullptr;

    ID3D12DescriptorHeap*     m_cullDescriptorHeap = nullptr;
    UINT                      m_descriptorSize = 0;

    HiZManager                m_hizManager;
    ClusterCullingConfig      m_config;
    ClusterCullingStats       m_stats;
    CameraCullConstants       m_cameraConstants = {};

    // Per-batch cluster draw mapping: objectId -> draw args range
    struct ObjectClusterDrawRange {
        uint32_t firstClusterIndex = 0;
        uint32_t clusterCount = 0;
        uint32_t visibleCount = 0;
    };
    std::unordered_map<int, ObjectClusterDrawRange> m_objectDrawRanges;

    UINT                      m_maxClusters = 65536;
    UINT                      m_currentClusterCount = 0;
    bool                      m_isInitialized = false;
    uint32_t                  m_lastRecordedVisible = 0;
    uint32_t                  m_lastRecordedCulled = 0;
};

} // namespace Eunoia
