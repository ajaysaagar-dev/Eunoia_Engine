#pragma once
// ============================================================================
// Eunoia Engine — Mesh Cluster Culling Plugin
// MeshClusterBuilder: Offline / Asset-Import meshlet cluster generation & caching
// ============================================================================

#include "MeshClusterTypes.h"
#include <EngineAssets/Geometry.h>
#include <unordered_map>
#include <mutex>
#include <string>

namespace Eunoia {

class MeshClusterBuilder {
public:
    static MeshClusterBuilder& Get();

    // Builds or retrieves cached clusters for a given PrimitiveMesh
    ClusteredMesh BuildOrGetClusters(
        const PrimitiveMesh& mesh,
        const std::string& assetKey,
        const ClusterCullingConfig& config = ClusterCullingConfig()
    );

    // Explicitly builds clusters without caching
    static ClusteredMesh BuildClusters(
        const PrimitiveMesh& mesh,
        const ClusterCullingConfig& config = ClusterCullingConfig()
    );

    // Invalidate cached clusters for a specific asset key or all assets
    void Invalidate(const std::string& assetKey);
    void ClearCache();

    // Check if clusters are cached for a key
    bool HasCachedClusters(const std::string& assetKey) const;

private:
    MeshClusterBuilder() = default;
    ~MeshClusterBuilder() = default;

    std::unordered_map<std::string, ClusteredMesh> m_clusterCache;
    mutable std::mutex m_cacheMutex;
};

} // namespace Eunoia
