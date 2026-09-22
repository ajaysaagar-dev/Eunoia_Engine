Analyze this C++17 / DirectX12 game engine repo (CMake-based) and
refactor it into the production-grade architecture defined below. Work
in a new branch `refactor/clean-architecture` and commit in small,
reviewable steps.

============================================================
TARGET ARCHITECTURE
============================================================

Layering (strict, one-directional — a module may only depend on modules
listed below it):

  App / Editor          <- application layer (Editor exe, future Runtime exe)
  EngineScene            <- ECS/scene graph, gameplay-facing API
  EngineRenderer          <- render passes, materials, camera, lighting
  EngineAssets            <- asset import/cook/load (meshes, textures, shaders)
  EngineRHI                <- render hardware interface (D3D12 abstraction)
  EnginePlatform             <- windowing, input, OS/file I/O abstraction
  EngineCore                    <- math, containers, memory, logging, jobs,
                                    events — zero dependencies on anything else

Rules:
  - EngineCore depends on nothing else in the engine (only the STL).
  - Each module exposes a public API only via include/<Module>/*.hpp;
    anything not meant for other modules lives in src/ and is not
    installed/exported.
  - EngineRHI wraps all raw D3D12 calls (ID3D12Device, command lists,
    descriptor heaps, fences) — no other module touches d3d12.h directly.
  - EngineRenderer talks to EngineRHI only through its abstraction, never
    to D3D12 directly — this is what would let a Vulkan/Metal RHI slot in
    later without touching Renderer/Scene/Editor.
  - EngineScene owns the ECS/actor hierarchy and drives EngineRenderer;
    it has no knowledge of D3D12 or ImGui.
  - Editor is the only module allowed to depend on Dear ImGui/ImGuizmo;
    it composes EngineScene + EngineRenderer into the tool you already
    have (Outliner/Details/Content Browser/Viewport).
  - Add a thin App/ or Runtime/ target later for a game-only exe with no
    editor/ImGui dependency — stub it now even if empty, so the
    separation exists from day one.

Directory layout to converge on:

  /
  ├── cmake/                     # shared CMake modules/helpers
  ├── docs/
  │   ├── ARCHITECTURE.md        # the layering rules above, written out
  │   └── dev.md                 # moved from root, updated for new layout
  ├── engine/
  │   ├── EngineCore/
  │   │   ├── include/EngineCore/
  │   │   ├── src/
  │   │   └── CMakeLists.txt
  │   ├── EnginePlatform/        # same include/ + src/ + CMakeLists.txt shape
  │   ├── EngineRHI/
  │   ├── EngineAssets/
  │   ├── EngineRenderer/
  │   └── EngineScene/
  ├── editor/
  │   └── Editor/                # include/Editor/ + src/ + CMakeLists.txt
  ├── runtime/                   # placeholder App target (no ImGui dep)
  ├── shaders/                   # unchanged, but confirm each module only
  │                                 references the shaders it owns
  ├── resources/icons/           # unchanged (editor-only resources)
  ├── tests/
  │   ├── EngineCore/
  │   ├── EngineRHI/
  │   └── ...                    # one test target per module, matching
  │                                 the engine/ tree above
  ├── tools/                     # build scripts (run.bat/build.bat moved
  │                                 here or kept at root — your call, but
  │                                 pick one and be consistent)
  ├── .github/workflows/         # add a basic CI build workflow if none
  │                                 exists (MSVC build + tests on push)
  ├── CMakeLists.txt             # top-level: add_subdirectory per module,
  │                                 in dependency order
  ├── .gitignore                 # extended to cover build/, obj/, .vs/,
  │                                 *.elogs, logs.elogs, etc.
  └── README.md

============================================================
WORK STEPS
============================================================

1. AUDIT FIRST (no changes yet). Map every existing file to its
   destination in the layout above. Flag anything ambiguous (e.g.
   Cube.cpp at root, logs.elogs, the current loose root-level
   include/ and src/) and propose where it goes or whether it's dead
   code to delete. Print this as a report and wait for my go-ahead
   before deleting anything.

2. MOVE, DON'T REWRITE. Relocate files into the target layout with
   git mv (preserve history). Do not change engine/rendering logic —
   this is structural only.

3. FIX INCLUDES & CMAKE. Update all #include paths for the new
   locations. Rewrite CMakeLists.txt per module with explicit
   PUBLIC/PRIVATE include dirs and target_link_libraries that mirror
   the dependency layering above exactly — fail the build (or flag it
   loudly) if a module tries to link "upward."

4. VERIFY. Confirm run.bat/build.bat (or their tools/ relocation)
   still produce Eunoia-Editor.exe. Update any paths they reference.

5. DOCUMENT. Write docs/ARCHITECTURE.md capturing the layering rules
   above, and update README.md's folder description to match.

6. REPORT. After each step, summarize what moved and why. End with a
   final tree view of the repo and a short list of any remaining
   ambiguities you didn't resolve, for me to decide.

Ask before deleting anything or before making a call you're not
confident about — don't guess on ambiguous ownership.

============================================================
REGRESSION NOTES
============================================================

### Directional Light Shadow Frustum & Contact Shadow Artifact Fix
- **Issue**: A flat, hard-edged dark artifact appeared detached from the floor/geometry near objects, disconnected from the ground plane.
- **Root Causes**:
  1. **Hardcoded Shadow Frustum**: `updateConstantBuffer()` in `Main.cpp` previously hardcoded `sceneCenter = (0,0,0)`, `orthoHalfSize = 14.0f`, `nearPlane = 1.0f`, `farPlane = 40.0f`. Any geometry extending beyond the 28x28 unit box around the origin was clipped by the shadow frustum.
  2. **Abrupt Out-of-Frustum Step**: In `CalculateShadow()`, coordinates outside NDC `[-1, 1]` or depth `[0, 1]` hit an early-return `1.0f`. This caused an instantaneous step change between shadowed (`1.0 - shadowStrength`) and unshadowed (`1.0f`), creating an unnatural razor-sharp dark boundary across geometry.
  3. **Degenerate Normal Determinant**: In `Scene.h` `BuildSceneMesh()`, non-uniform or degenerate scaling previously had a fallback that collapsed normals.
- **Fix Applied**:
  1. **Dynamic Scene Bounds Fitting**: Added `Scene::GetSceneAABB()` to compute the world-space bounding box of all visible mesh geometry. `updateConstantBuffer()` now computes `sceneCenter`, `sceneRadius`, and sets `orthoHalfSize = sceneRadius * 1.25f + 2.0f`, `lightDistance = sceneRadius * 2.0f + 10.0f`, and `farPlane = lightDistance + sceneRadius * 2.0f + 10.0f`.
  2. **Frustum Border Soft Edge Fading**: Added border distance smooth fading (`fade = min(borderDist * 10.0f, zBorderDist * 10.0f)`) in `CalculateShadow()` so points approaching the shadow frustum boundary fade smoothly to 1.0f instead of hard clipping.
  3. **Debug Frustum Visualization**: Added `showLightFrustum` wireframe rendering of the 12 edges of the directional light frustum in world space, with a toggle under the Viewport "Show" menu and the Directional Light Inspector.
  4. **Degenerate Normal Matrix Fallback**: Extracted orthonormal basis using Gram-Schmidt in `BuildSceneMesh()` when matrix determinant is near-zero.
  5. **Empty Scene Default**: New levels and empty scenes initialize completely empty (no objects, no point lights, 0 light/ambient intensity).