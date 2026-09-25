#pragma once
// ============================================================================
// Eunoia Engine — Mesh Cluster Culling Plugin
// Types, structures, and configuration for GPU meshlet/cluster culling
// ============================================================================

#include <cstdint>
#include <vector>
#include <string>
#include <glm/glm.hpp>

namespace Eunoia {

// CPU Representation of a single mesh cluster / meshlet
struct MeshCluster {
    uint32_t  vertexOffset = 0;       // Base vertex offset in source mesh
    uint32_t  vertexCount = 0;        // Number of unique vertices referenced
    uint32_t  indexOffset = 0;        // Starting index in mesh index buffer
    uint32_t  indexCount = 0;         // Number of indices (triangles * 3)
    uint32_t  triangleCount = 0;      // 32 to 128 triangles

    // Local-space spatial bounds
    glm::vec3 boundsMin{0.0f};        // Local AABB min
    glm::vec3 boundsMax{0.0f};        // Local AABB max
    glm::vec3 sphereCenter{0.0f};     // Bounding sphere center
    float     sphereRadius = 0.0f;    // Bounding sphere radius

    // Normal cone data for backface / normal-cone culling
    glm::vec3 coneApex{0.0f};         // Apex of normal cone
    glm::vec3 coneAxis{0.0f, 1.0f, 0.0f}; // Average normal direction
    float     coneCutoff = 1.0f;      // cos(half_angle) cutoff for culling

    int       lodLevel = 0;           // LOD index (0 = highest detail)
    int       materialIndex = 0;      // Material index or section ID
    int       clusterId = 0;          // Unique index within the mesh
};

// Clustered representation of a full mesh
struct ClusteredMesh {
    std::string              sourceMeshPath;
    uint32_t                 totalTriangles = 0;
    uint32_t                 totalVertices = 0;
    std::vector<MeshCluster> clusters;
    bool                     isValid = false;
};

// GPU-aligned struct layout (144 bytes, 16-byte aligned for HLSL StructuredBuffer)
struct alignas(16) GPUClusterData {
    glm::vec3 boundsMin;
    float     sphereRadius;
    glm::vec3 boundsMax;
    uint32_t  indexOffset;
    glm::vec3 sphereCenter;
    uint32_t  indexCount;
    glm::vec3 coneApex;
    float     coneCutoff;
    glm::vec3 coneAxis;
    uint32_t  baseVertex;
    glm::mat4 worldMatrix;
};

// Constant buffer for camera and culling settings (256-byte aligned)
struct alignas(256) CameraCullConstants {
    glm::mat4 viewProj;
    glm::vec4 frustumPlanes[6];       // Left, Right, Bottom, Top, Near, Far: (nx, ny, nz, d)
    glm::vec3 cameraPos;
    float     minScreenSize = 2.0f;   // In pixels (diameter threshold)
    glm::vec2 viewportSize{1280.0f, 720.0f};
    glm::vec2 hizSize{1024.0f, 1024.0f};
    uint32_t  totalClusters = 0;
    uint32_t  enableFrustum = 1;
    uint32_t  enableBackface = 1;
    uint32_t  enableHiZ = 1;
    uint32_t  enableScreenSize = 1;
    uint32_t  debugMode = 0;          // 0 = Normal, 1 = Show Clusters, 2 = Bounds, 3 = Visible, 4 = Culled, 5 = Hi-Z, 6 = IDs
    float     hizMaxMip = 0.0f;
    float     padding = 0.0f;
};

// Configuration options for Mesh Cluster Culling
struct ClusterCullingConfig {
    bool enabled = true;
    int  clusterMinTriangles = 32;
    int  clusterMaxTriangles = 128;
    int  targetClusterTriangles = 64;
    bool enableFrustumCulling = true;
    bool enableBackfaceCulling = true;
    bool enableHiZOcclusion = true;
    bool enableScreenSizeCulling = true;
    bool enableIndirectRendering = true;
    int  debugVisualizationMode = 0;   // 0: None, 1: Show Clusters, 2: Show Bounds, 3: Show Visible, 4: Show Culled, 5: Show Hi-Z, 6: Show IDs
    float minScreenSizePixels = 2.0f;
};

// Real-time statistics exposed to Editor Details and Performance panels
struct ClusterCullingStats {
    uint32_t objectsEnabled = 0;
    uint32_t totalClusters = 0;
    uint32_t visibleClusters = 0;
    uint32_t culledClusters = 0;
    float    visibilityRatio = 0.0f;  // (visible / total) * 100%
    float    cullingRatio = 0.0f;     // (culled / total) * 100%
    float    cpuCullingTimeMs = 0.0f;
    float    gpuCullingTimeMs = 0.0f;
    uint32_t indirectDraws = 0;

    void Reset() {
        objectsEnabled = 0;
        totalClusters = 0;
        visibleClusters = 0;
        culledClusters = 0;
        visibilityRatio = 0.0f;
        cullingRatio = 0.0f;
        cpuCullingTimeMs = 0.0f;
        gpuCullingTimeMs = 0.0f;
        indirectDraws = 0;
    }

    void Finalize() {
        if (totalClusters > 0) {
            visibilityRatio = (float)visibleClusters / (float)totalClusters * 100.0f;
            cullingRatio = (float)culledClusters / (float)totalClusters * 100.0f;
        } else {
            visibilityRatio = 0.0f;
            cullingRatio = 0.0f;
        }
    }
};

// Per-object cluster visibility statistics for Details Panel
struct ObjectClusterStats {
    bool     enabled = false;
    uint32_t totalClusters = 0;
    uint32_t visibleClusters = 0;
    uint32_t culledClusters = 0;
    float    cullingRatio = 0.0f;
};

} // namespace Eunoia
