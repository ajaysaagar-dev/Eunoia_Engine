# Mesh-Cluster-Culling Plugin

The **`Mesh-Cluster-Culling`** plugin provides GPU-driven meshlet and cluster visibility culling for the Eunoia Engine DirectX 12 renderer.

## Features

- **Granular Cluster Culling**: Meshes are partitioned into 32–128 triangle clusters with spatial bounding boxes, bounding spheres, and normal cones.
- **Per-Object Property**: Default is **OFF** for all meshes and actors. Fully opt-in.
- **Parent-to-Child Recursive Propagation**: Enabling on any parent actor recursively activates culling across all descendant actors in the hierarchy.
- **GPU-Driven Pipeline**:
  1. Frustum Culling
  2. Backface / Normal-Cone Culling
  3. Screen-Space Projected Size Testing (subpixel geometry culling)
  4. Hi-Z Occlusion Culling (depth pyramid mip downsampling)
  5. GPU Indirect Draw Submission (`ExecuteIndirect`)
- **Dual Camera Support**: Automatically tracks Editor Viewport Camera in edit mode and active level camera in play/game mode.
- **Zero Overhead when Disabled**: Objects with `Mesh Cluster Culling = OFF` bypass all compute dispatches and render via standard rasterization without regression.
- **Real-Time Statistics & Debugging**:
  - Inspector details panel shows cluster counts, visible clusters, culled clusters, and culling ratio.
  - Optional debug visualization modes (Show Clusters, Show Bounds, Show Visible, Show Culled, Show Hi-Z, Show Cluster IDs).

## Configuration

Settings can be inspected and altered at runtime or through configuration:

```ini
[MeshClusterCulling]
Enabled=true
ClusterMinTriangles=32
ClusterMaxTriangles=128
EnableFrustumCulling=true
EnableBackfaceCulling=true
EnableHiZOcclusion=true
EnableScreenSizeCulling=true
EnableIndirectRendering=true
DebugVisualization=false
```
