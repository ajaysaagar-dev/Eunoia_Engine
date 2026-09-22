// ============================================================================
// Eunoia Engine — Mesh Cluster Culling Plugin
// HiZBuild.hlsl: Hierarchical-Z Depth Pyramid Downsampling Compute Shader
// ============================================================================

cbuffer HiZBuildConstants : register(b0)
{
    float2 srcTexelSize;
    float2 dstTexelSize;
    uint2  dstMipSize;
    uint   isFirstMip; // 1 if copying from depth buffer, 0 if downsampling
};

Texture2D<float>   SrcMip : register(t0);
RWTexture2D<float> DstMip : register(u0);

[numthreads(8, 8, 1)]
void CSMain(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (dispatchThreadId.x >= dstMipSize.x || dispatchThreadId.y >= dstMipSize.y)
        return;

    uint2 dstCoord = dispatchThreadId.xy;

    if (isFirstMip != 0)
    {
        // Direct copy or 2x2 max downsample from base depth buffer
        uint2 srcCoord = dstCoord * 2;
        float d0 = SrcMip.Load(int3(srcCoord, 0));
        float d1 = SrcMip.Load(int3(srcCoord + uint2(1, 0), 0));
        float d2 = SrcMip.Load(int3(srcCoord + uint2(0, 1), 0));
        float d3 = SrcMip.Load(int3(srcCoord + uint2(1, 1), 0));

        // Standard Z (0 = near, 1 = far): conservative occluder depth across tile is MAX
        float maxDepth = max(max(d0, d1), max(d2, d3));
        DstMip[dstCoord] = maxDepth;
    }
    else
    {
        // Subsequent mip levels: 2x2 reduction
        uint2 srcCoord = dstCoord * 2;
        float d0 = SrcMip.Load(int3(srcCoord, 0));
        float d1 = SrcMip.Load(int3(srcCoord + uint2(1, 0), 0));
        float d2 = SrcMip.Load(int3(srcCoord + uint2(0, 1), 0));
        float d3 = SrcMip.Load(int3(srcCoord + uint2(1, 1), 0));

        float maxDepth = max(max(d0, d1), max(d2, d3));
        DstMip[dstCoord] = maxDepth;
    }
}
