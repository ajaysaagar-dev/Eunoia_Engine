// ============================================================================
// Eunoia Engine — Mesh Cluster Culling Plugin
// MeshClusterBuilder: Implementation of meshlet cluster generation & caching
// ============================================================================

#include "MeshClusterCulling/MeshClusterBuilder.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

namespace Eunoia {

MeshClusterBuilder& MeshClusterBuilder::Get() {
    static MeshClusterBuilder s_instance;
    return s_instance;
}

ClusteredMesh MeshClusterBuilder::BuildOrGetClusters(
    const PrimitiveMesh& mesh,
    const std::string& assetKey,
    const ClusterCullingConfig& config)
{
    if (!assetKey.empty()) {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        auto it = m_clusterCache.find(assetKey);
        if (it != m_clusterCache.end() && it->second.isValid) {
            return it->second;
        }
    }

    ClusteredMesh clustered = BuildClusters(mesh, config);
    clustered.sourceMeshPath = assetKey;

    if (!assetKey.empty() && clustered.isValid) {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        m_clusterCache[assetKey] = clustered;
    }

    return clustered;
}

void MeshClusterBuilder::Invalidate(const std::string& assetKey) {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_clusterCache.erase(assetKey);
}

void MeshClusterBuilder::ClearCache() {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_clusterCache.clear();
}

bool MeshClusterBuilder::HasCachedClusters(const std::string& assetKey) const {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    auto it = m_clusterCache.find(assetKey);
    return (it != m_clusterCache.end() && it->second.isValid);
}

ClusteredMesh MeshClusterBuilder::BuildClusters(
    const PrimitiveMesh& mesh,
    const ClusterCullingConfig& config)
{
    ClusteredMesh result;
    result.isValid = false;

    if (mesh.vertices.empty() || mesh.indices.empty()) {
        std::cout << "[MeshClusterCulling] Warning: Cannot build clusters for empty mesh." << std::endl;
        return result;
    }

    size_t indexCount = mesh.indices.size();
    if (indexCount % 3 != 0) {
        std::cout << "[MeshClusterCulling] Warning: Index count not divisible by 3 (" << indexCount << ")." << std::endl;
        return result;
    }

    uint32_t totalTriangles = (uint32_t)(indexCount / 3);
    result.totalTriangles = totalTriangles;
    result.totalVertices = (uint32_t)mesh.vertices.size();

    // Determine cluster size bounds
    int minTris = std::max(16, config.clusterMinTriangles);
    int maxTris = std::max(minTris, config.clusterMaxTriangles);
    int targetTris = std::clamp(config.targetClusterTriangles, minTris, maxTris);

    struct TriangleInfo {
        uint32_t triIndex;
        glm::vec3 centroid;
        glm::vec3 normal;
    };

    std::vector<TriangleInfo> triangles(totalTriangles);
    glm::vec3 meshMin(std::numeric_limits<float>::max());
    glm::vec3 meshMax(-std::numeric_limits<float>::max());

    for (uint32_t t = 0; t < totalTriangles; ++t) {
        uint32_t i0 = mesh.indices[t * 3 + 0];
        uint32_t i1 = mesh.indices[t * 3 + 1];
        uint32_t i2 = mesh.indices[t * 3 + 2];

        if (i0 >= mesh.vertices.size()) i0 = 0;
        if (i1 >= mesh.vertices.size()) i1 = 0;
        if (i2 >= mesh.vertices.size()) i2 = 0;

        const glm::vec3& v0 = mesh.vertices[i0].pos;
        const glm::vec3& v1 = mesh.vertices[i1].pos;
        const glm::vec3& v2 = mesh.vertices[i2].pos;

        triangles[t].triIndex = t;
        triangles[t].centroid = (v0 + v1 + v2) / 3.0f;

        glm::vec3 e1 = v1 - v0;
        glm::vec3 e2 = v2 - v0;
        glm::vec3 n = glm::cross(e1, e2);
        float len = glm::length(n);
        triangles[t].normal = (len > 1e-6f) ? (n / len) : glm::vec3(0.0f, 1.0f, 0.0f);

        meshMin = glm::min(meshMin, glm::min(v0, glm::min(v1, v2)));
        meshMax = glm::max(meshMax, glm::max(v0, glm::max(v1, v2)));
    }

    // Determine principal axis for spatial sorting
    glm::vec3 meshExtent = meshMax - meshMin;
    int sortAxis = 0; // X
    if (meshExtent.y > meshExtent.x && meshExtent.y >= meshExtent.z) {
        sortAxis = 1; // Y
    } else if (meshExtent.z > meshExtent.x && meshExtent.z > meshExtent.y) {
        sortAxis = 2; // Z
    }

    // Spatial sort triangles along dominant axis to maximize cluster spatial locality
    std::sort(triangles.begin(), triangles.end(), [sortAxis](const TriangleInfo& a, const TriangleInfo& b) {
        return a.centroid[sortAxis] < b.centroid[sortAxis];
    });

    // Partition sorted triangles into clusters
    uint32_t triOffset = 0;
    int clusterId = 0;

    while (triOffset < totalTriangles) {
        uint32_t remaining = totalTriangles - triOffset;
        uint32_t count = std::min((uint32_t)targetTris, remaining);

        // If remaining after this is less than minTris, merge into current cluster
        if (remaining - count < (uint32_t)minTris && (remaining - count) > 0) {
            if (count + (remaining - count) <= (uint32_t)maxTris) {
                count = remaining;
            }
        }

        MeshCluster cluster;
        cluster.clusterId = clusterId++;
        cluster.indexOffset = triOffset * 3;
        cluster.indexCount = count * 3;
        cluster.triangleCount = count;
        cluster.vertexOffset = 0;

        glm::vec3 bMin(std::numeric_limits<float>::max());
        glm::vec3 bMax(-std::numeric_limits<float>::max());
        glm::vec3 avgNormal(0.0f);

        for (uint32_t i = 0; i < count; ++i) {
            uint32_t t = triangles[triOffset + i].triIndex;
            uint32_t i0 = mesh.indices[t * 3 + 0];
            uint32_t i1 = mesh.indices[t * 3 + 1];
            uint32_t i2 = mesh.indices[t * 3 + 2];

            if (i0 >= mesh.vertices.size()) i0 = 0;
            if (i1 >= mesh.vertices.size()) i1 = 0;
            if (i2 >= mesh.vertices.size()) i2 = 0;

            const glm::vec3& v0 = mesh.vertices[i0].pos;
            const glm::vec3& v1 = mesh.vertices[i1].pos;
            const glm::vec3& v2 = mesh.vertices[i2].pos;

            bMin = glm::min(bMin, glm::min(v0, glm::min(v1, v2)));
            bMax = glm::max(bMax, glm::max(v0, glm::max(v1, v2)));

            avgNormal += triangles[triOffset + i].normal;
        }

        cluster.boundsMin = bMin;
        cluster.boundsMax = bMax;
        cluster.sphereCenter = (bMin + bMax) * 0.5f;

        // Compute tight sphere radius
        float maxDistSq = 0.0f;
        for (uint32_t i = 0; i < count; ++i) {
            uint32_t t = triangles[triOffset + i].triIndex;
            for (int k = 0; k < 3; ++k) {
                uint32_t vi = mesh.indices[t * 3 + k];
                if (vi >= mesh.vertices.size()) vi = 0;
                glm::vec3 diff = mesh.vertices[vi].pos - cluster.sphereCenter;
                maxDistSq = std::max(maxDistSq, glm::dot(diff, diff));
            }
        }
        cluster.sphereRadius = std::sqrt(maxDistSq);

        // Compute normal cone
        float avgNormLen = glm::length(avgNormal);
        if (avgNormLen > 1e-5f) {
            cluster.coneAxis = avgNormal / avgNormLen;
        } else {
            cluster.coneAxis = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        cluster.coneApex = cluster.sphereCenter;

        // Compute minimum cosine of angle between coneAxis and face normals
        float minCos = 1.0f;
        for (uint32_t i = 0; i < count; ++i) {
            float dotVal = glm::dot(cluster.coneAxis, triangles[triOffset + i].normal);
            minCos = std::min(minCos, dotVal);
        }
        cluster.coneCutoff = minCos;

        result.clusters.push_back(cluster);
        triOffset += count;
    }

    result.isValid = !result.clusters.empty();
    return result;
}

} // namespace Eunoia
