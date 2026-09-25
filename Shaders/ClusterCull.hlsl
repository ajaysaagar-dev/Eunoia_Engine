// ============================================================================
// Eunoia Engine — Mesh Cluster Culling Plugin
// ClusterCull.hlsl: GPU-Driven Meshlet/Cluster Visibility Culling Compute Shader
// ============================================================================

struct GPUClusterData
{
    float3 boundsMin;
    float  sphereRadius;
    float3 boundsMax;
    uint   indexOffset;
    float3 sphereCenter;
    uint   indexCount;
    float3 coneApex;
    float  coneCutoff;
    float3 coneAxis;
    uint   baseVertex;
    float4x4 worldMatrix;
};

// D3D12_DRAW_INDEXED_ARGUMENTS (5 uints)
struct DrawIndexedArguments
{
    uint IndexCountPerInstance;
    uint InstanceCount;
    uint StartIndexLocation;
    int  BaseVertexLocation;
    uint StartInstanceLocation;
};

cbuffer CameraCullData : register(b0)
{
    float4x4 g_viewProj;
    float4   g_frustumPlanes[6]; // Left, Right, Bottom, Top, Near, Far: (nx, ny, nz, d)
    float3   g_cameraPos;
    float    g_minScreenSize;    // In pixels
    float2   g_viewportSize;
    float2   g_hizSize;
    uint     g_totalClusters;
    uint     g_enableFrustum;
    uint     g_enableBackface;
    uint     g_enableHiZ;
    uint     g_enableScreenSize;
    uint     g_debugMode;        // 0: Normal, 1: Show Clusters, etc.
    float    g_hizMaxMip;
    float    g_padding;
};

StructuredBuffer<GPUClusterData>            g_clusters           : register(t0);
Texture2D<float>                           g_hizPyramid         : register(t1);
SamplerState                               g_pointSampler       : register(s0);

RWStructuredBuffer<uint>                   g_visibleClusterIds  : register(u0);
RWStructuredBuffer<DrawIndexedArguments>   g_indirectArgs       : register(u1);
RWStructuredBuffer<uint>                   g_visibilityCounters : register(u2); // [0] = visibleCount, [1] = culledCount

bool IsInFrustum(float3 worldCenter, float radius)
{
    [unroll]
    for (int i = 0; i < 6; ++i)
    {
        float dist = dot(g_frustumPlanes[i].xyz, worldCenter) + g_frustumPlanes[i].w;
        if (dist < -radius)
        {
            return false;
        }
    }
    return true;
}

bool IsBackfacing(float3 worldApex, float3 worldAxis, float cutoff)
{
    float3 toCamera = g_cameraPos - worldApex;
    float dist = length(toCamera);
    if (dist < 1e-4) return false;
    toCamera /= dist;

    // If cone angle is wide (cutoff < 0), cluster can see both front and back
    if (cutoff < -0.99) return false;

    // Normal cone test: if dot(toCamera, axis) < -sin(half_angle), all normals point away
    float sinHalfAngle = sqrt(max(0.0, 1.0 - cutoff * cutoff));
    return (dot(toCamera, worldAxis) < -sinHalfAngle);
}

[numthreads(64, 1, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint clusterIdx = dispatchThreadId.x;
    if (clusterIdx >= g_totalClusters)
        return;

    GPUClusterData cluster = g_clusters[clusterIdx];

    // Transform bounding sphere to world space
    float4 worldCenter4 = mul(float4(cluster.sphereCenter, 1.0), cluster.worldMatrix);
    float3 worldCenter = worldCenter4.xyz;

    // Compute max scale factor from matrix columns
    float scaleX = length(cluster.worldMatrix[0].xyz);
    float scaleY = length(cluster.worldMatrix[1].xyz);
    float scaleZ = length(cluster.worldMatrix[2].xyz);
    float maxScale = max(scaleX, max(scaleY, scaleZ));
    float worldRadius = cluster.sphereRadius * maxScale;

    // 1. Frustum Culling
    if (g_enableFrustum != 0)
    {
        if (!IsInFrustum(worldCenter, worldRadius))
        {
            InterlockedAdd(g_visibilityCounters[1], 1);
            return;
        }
    }

    // 2. Backface / Normal-Cone Culling
    if (g_enableBackface != 0 && cluster.coneCutoff > -0.95)
    {
        float3 worldApex = mul(float4(cluster.coneApex, 1.0), cluster.worldMatrix).xyz;
        float3 worldAxis = normalize(mul(float4(cluster.coneAxis, 0.0), cluster.worldMatrix).xyz);
        if (IsBackfacing(worldApex, worldAxis, cluster.coneCutoff))
        {
            InterlockedAdd(g_visibilityCounters[1], 1);
            return;
        }
    }

    // 3. Screen-Space Projected Size Test
    float distToCam = distance(g_cameraPos, worldCenter);
    if (g_enableScreenSize != 0 && distToCam > worldRadius)
    {
        // Projected pixel diameter estimation
        float projFactor = g_viewProj[1][1] * 0.5 * g_viewportSize.y;
        float pixelDiameter = (worldRadius * 2.0 / distToCam) * projFactor;
        if (pixelDiameter < g_minScreenSize)
        {
            InterlockedAdd(g_visibilityCounters[1], 1);
            return;
        }
    }

    // 4. Hi-Z Occlusion Culling
    if (g_enableHiZ != 0 && g_hizMaxMip > 0.0)
    {
        // Project 8 corners of the world-space bounding box to screen space
        float3 bMin = cluster.boundsMin;
        float3 bMax = cluster.boundsMax;
        float3 corners[8] = {
            mul(float4(bMin.x, bMin.y, bMin.z, 1.0), cluster.worldMatrix).xyz,
            mul(float4(bMax.x, bMin.y, bMin.z, 1.0), cluster.worldMatrix).xyz,
            mul(float4(bMin.x, bMax.y, bMin.z, 1.0), cluster.worldMatrix).xyz,
            mul(float4(bMax.x, bMax.y, bMin.z, 1.0), cluster.worldMatrix).xyz,
            mul(float4(bMin.x, bMin.y, bMax.z, 1.0), cluster.worldMatrix).xyz,
            mul(float4(bMax.x, bMin.y, bMax.z, 1.0), cluster.worldMatrix).xyz,
            mul(float4(bMin.x, bMax.y, bMax.z, 1.0), cluster.worldMatrix).xyz,
            mul(float4(bMax.x, bMax.y, bMax.z, 1.0), cluster.worldMatrix).xyz
        };

        float minZ = 1.0;
        float2 minUV = float2(1.0, 1.0);
        float2 maxUV = float2(0.0, 0.0);
        bool behindCamera = false;

        [unroll]
        for (int c = 0; c < 8; ++c)
        {
            float4 clipPos = mul(float4(corners[c], 1.0), g_viewProj);
            if (clipPos.w <= 1e-4)
            {
                behindCamera = true;
                break;
            }
            float3 ndc = clipPos.xyz / clipPos.w;
            float2 uv = ndc.xy * float2(0.5, -0.5) + 0.5;

            minUV = min(minUV, uv);
            maxUV = max(maxUV, uv);
            minZ = min(minZ, ndc.z);
        }

        if (!behindCamera && minUV.x < 1.0 && maxUV.x > 0.0 && minUV.y < 1.0 && maxUV.y > 0.0)
        {
            minUV = saturate(minUV);
            maxUV = saturate(maxUV);

            float2 rectSize = (maxUV - minUV) * g_hizSize;
            float maxDim = max(rectSize.x, rectSize.y);
            float mip = clamp(ceil(log2(max(1.0, maxDim))), 0.0, g_hizMaxMip);

            // Sample Hi-Z depth at conservative corners
            float d0 = g_hizPyramid.SampleLevel(g_pointSampler, minUV, mip);
            float d1 = g_hizPyramid.SampleLevel(g_pointSampler, float2(maxUV.x, minUV.y), mip);
            float d2 = g_hizPyramid.SampleLevel(g_pointSampler, float2(minUV.x, maxUV.y), mip);
            float d3 = g_hizPyramid.SampleLevel(g_pointSampler, maxUV, mip);

            // In standard Z (0 = near, 1 = far), max is furthest occluder in tile
            float maxOccluderDepth = max(max(d0, d1), max(d2, d3));

            // Small epsilon bias to prevent self-occlusion artifacts
            const float depthBias = 0.0005;
            if (minZ > (maxOccluderDepth + depthBias))
            {
                // Cluster is strictly behind occluders -> Cull
                InterlockedAdd(g_visibilityCounters[1], 1);
                return;
            }
        }
    }

    // Cluster is VISIBLE!
    uint visibleSlot;
    InterlockedAdd(g_visibilityCounters[0], 1, visibleSlot);

    g_visibleClusterIds[visibleSlot] = clusterIdx;

    // Fill indirect draw arguments
    DrawIndexedArguments args;
    args.IndexCountPerInstance = cluster.indexCount;
    args.InstanceCount = 1;
    args.StartIndexLocation = cluster.indexOffset;
    args.BaseVertexLocation = (int)cluster.baseVertex;
    args.StartInstanceLocation = 0;

    g_indirectArgs[visibleSlot] = args;
}
