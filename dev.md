Eunoia-Engine/
├── CMakeLists.txt                 # top-level, adds subdirectories below
├── deps/                          # third-party (imgui, glfw, glm, ufbx, stb, ...) — unchanged
│
├── EngineCore/                    # STATIC LIB — no D3D12, no windowing
│   ├── include/EngineCore/
│   │   ├── Log.h                  # replaces std::cerr/std::cout calls
│   │   ├── Assert.h
│   │   ├── Time.h
│   │   ├── JobSystem.h            # thread pool for async loads
│   │   ├── Result.h               # error type instead of bare bool/HRESULT
│   │   └── UUID.h / AssetID.h
│   └── src/...
│
├── EnginePlatform/                 # STATIC LIB — windowing/input only
│   ├── include/EnginePlatform/
│   │   ├── Window.h                # wraps GLFW window creation/callbacks
│   │   └── InputSystem.h           # moved from include/InputSystem.h
│   └── src/...
│
├── EngineRHI/                      # STATIC LIB — the ONLY place that
│   │                                # #includes <d3d12.h>/<dxgi1_4.h>
│   ├── include/EngineRHI/
│   │   ├── Device.h                 # device + adapter selection (was
│   │   │                            #   the top of initD3D12)
│   │   ├── SwapChain.h              # ResizeBuffers, Present, RTVs
│   │   ├── CommandContext.h         # command list/allocator + Reset/Close
│   │   ├── Fence.h                  # SafeWaitForFence lives here, and on
│   │   │                            #   real device-removal it raises an
│   │   │                            #   OnDeviceLost event (see EditorApp)
│   │   ├── DescriptorAllocator.h    # bounds-checked ring/free-list
│   │   │                            #   allocator — the fix for the SRV
│   │   │                            #   heap overflow lives here, in ONE
│   │   │                            #   place, instead of 3 call sites
│   │   ├── UploadHeap.h             # UploadTextureToD3D12 + upload fence
│   │   └── PipelineState.h          # shader compile + PSO creation
│   └── src/...
│
├── EngineRenderer/                  # STATIC LIB — depends on RHI, knows
│   │                                 # about Scene but not about ImGui
│   ├── include/EngineRenderer/
│   │   ├── SceneRenderer.h          # shadow pass + main pass, was inline
│   │   │                            #   in renderFrame()
│   │   ├── MaterialSystem.h         # GetOrCreateMaterialTable logic
│   │   ├── TextureManager.h         # moved as-is, now uses JobSystem for
│   │   │                            #   decode (stb_image) off the render
│   │   │                            #   thread — this is the fix for
│   │   │                            #   "import freezes the UI"
│   │   └── ConstantBuffers.h
│   └── src/...
│
├── EngineScene/                     # STATIC LIB — no rendering/platform
│   ├── include/EngineScene/
│   │   ├── GameObject.h             # moved as-is
│   │   ├── Scene.h                  # moved as-is
│   │   ├── Camera.h
│   │   ├── Level.h
│   │   └── SceneSerializer.h        # see "Serialization" note below
│   └── src/...
│
├── EngineAssets/                    # STATIC LIB — depends on Core+Scene
│   ├── include/EngineAssets/
│   │   ├── AssetID.h / AssetPath.h / AssetType.h / AssetMetadata.h
│   │   ├── AssetRegistry.h
│   │   ├── AssetManager.h
│   │   ├── SoftAssetReference.h
│   │   └── Importers/
│   │       ├── MeshImporter.h       # dispatches to below
│   │       ├── ObjImporter.h
│   │       ├── GltfImporter.h
│   │       └── FbxImporter.h        # the ufbx-based one, split out
│   └── src/...
│
├── Editor/                          # EXECUTABLE (links all Engine* libs
│   │                                 # + ImGui/ImGuizmo) — this is where
│   │                                 # EngineUI.cpp gets split up
│   ├── include/Editor/
│   │   ├── EditorApp.h              # owns the main loop (was in main());
│   │   │                            #   listens for RHI "device lost" and
│   │   │                            #   shows the restart-required overlay
│   │   ├── Panels/
│   │   │   ├── OutlinerPanel.h
│   │   │   ├── DetailsPanel.h
│   │   │   ├── ContentBrowserPanel.h
│   │   │   ├── ViewportPanel.h      # gizmos + fly camera input live here
│   │   │   ├── ConsolePanel.h       # AddLog / Output Log, was g_engineUI.AddLog
│   │   │   └── WorldSettingsPanel.h
│   │   └── Theme.h                  # SetupTheme()
│   └── src/... (one .cpp per panel above, each a few hundred lines
│                instead of one 184KB file)
│
├── Game/                            # EXECUTABLE (optional, future) — a
│   │                                 # thin runtime that links EngineCore/
│   │                                 # RHI/Renderer/Scene/Assets but NOT
│   │                                 # Editor/ImGui, for shipping games
│   └── src/main.cpp
│
├── resources/                       # unchanged
├── shaders/                         # unchanged
└── tests/                           # NEW — unit tests for the pieces that
    ├── EngineCore.tests/            #   don't need a GPU: JobSystem,
    ├── EngineScene.tests/           #   AssetRegistry, SceneSerializer
    └── EngineAssets.tests/          #   parsing, DescriptorAllocator bounds


# Fix: Gizmo manipulation causes GPU hang (0x887a0006 DEVICE_HUNG)

## Root cause

The previous fix (commit `c639f64`, "Fix GPU device loss 0x887a0006...")
added `determinant`/`normalize`-zero guards to several matrix-inverse call
sites (`Scene.h` reparenting, normal-matrix calc, `PickObjectAtCursor`), but
**missed the actual leak**: `EngineUI::RenderGizmo()` in `src/EngineUI.cpp`.

```cpp
// src/EngineUI.cpp, inside RenderGizmo(), ~line 3330
glm::mat4 localMatrix = modelMatrix;
if (obj->parentId != -1) {
    glm::mat4 parentWorld = scene.GetWorldMatrix(obj->parentId);
    localMatrix = glm::inverse(parentWorld) * modelMatrix;   // no singularity check
}
...
} else if (currentGizmoOperation == ImGuizmo::SCALE) {
    obj->scale = glm::vec3(
        glm::length(glm::vec3(localMatrix[0])),               // can become NaN
        glm::length(glm::vec3(localMatrix[1])),
        glm::length(glm::vec3(localMatrix[2]))
    );
}
```

If the selected object's parent has (or ever had, and saved to disk) a
degenerate/near-zero-scale transform, `glm::inverse(parentWorld)` silently
produces `Inf`/`NaN` instead of erroring. That NaN flows into
`obj->scale`/`position`/`rotation` on the `GameObject`, gets saved back to
the scene, and every frame afterward `RenderBatch::BuildFromScene` in
`include/Scene.h` computes:

```cpp
glm::vec4 worldPos = model * glm::vec4(mv.pos, 1.0f); // NOT guarded
```

NaN clip-space positions in the vertex buffer are a known trigger for
`DXGI_ERROR_DEVICE_HUNG` — the GPU rasterizer chokes on them, Windows' TDR
kills the context after ~2s, and the app hits the "device lost" path added
in the last fix (which now correctly stops rendering — but the underlying
corruption is never cleaned up, so it keeps happening on that object).

## Task 1 (critical): guard the parent-inverse in `RenderGizmo`

In `src/EngineUI.cpp`, `EngineUI::RenderGizmo()`, replace:
```cpp
glm::mat4 localMatrix = modelMatrix;
if (obj->parentId != -1) {
    glm::mat4 parentWorld = scene.GetWorldMatrix(obj->parentId);
    localMatrix = glm::inverse(parentWorld) * modelMatrix;
}
```
with:
```cpp
glm::mat4 localMatrix = modelMatrix;
if (obj->parentId != -1) {
    glm::mat4 parentWorld = scene.GetWorldMatrix(obj->parentId);
    float parentDet = glm::determinant(parentWorld);
    if (std::abs(parentDet) > 1e-6f && !std::isnan(parentDet)) {
        localMatrix = glm::inverse(parentWorld) * modelMatrix;
    }
    // else: parent transform is degenerate — fall back to using modelMatrix
    // as-is (world space) rather than propagating Inf/NaN into localMatrix.
}
```

## Task 2: clamp/guard the scale extraction right after it

Still in `RenderGizmo()`, after computing `localMatrix`, guard the SCALE
branch so a NaN or non-finite length never reaches `obj->scale`:
```cpp
} else if (currentGizmoOperation == ImGuizmo::SCALE) {
    glm::vec3 newScale(
        glm::length(glm::vec3(localMatrix[0])),
        glm::length(glm::vec3(localMatrix[1])),
        glm::length(glm::vec3(localMatrix[2]))
    );
    bool valid = !std::isnan(newScale.x) && !std::isnan(newScale.y) && !std::isnan(newScale.z)
              && std::isfinite(newScale.x) && std::isfinite(newScale.y) && std::isfinite(newScale.z);
    if (valid) {
        // Also clamp to a small positive minimum so scale can never hit exactly
        // 0 (which would make THIS object's own world matrix singular for any
        // future children/gizmo use).
        const float kMinScale = 1e-4f;
        obj->scale = glm::max(newScale, glm::vec3(kMinScale));
    }
    // else: ignore this frame's gizmo update rather than corrupting obj->scale.
}
```
Apply the same `isnan`/`isfinite` check before assigning `obj->position` and
`obj->rotation` in the TRANSLATE/ROTATE/default branches of the same
function, for the same reason — a bad `localMatrix` from
`ImGuizmo::DecomposeMatrixToComponents` should be dropped, not applied.

## Task 3: guard the actual vertex-transform line (defense in depth)

In `include/Scene.h`, `RenderBatch::BuildFromScene` (the loop that fills
`outVertices`), the line:
```cpp
glm::vec4 worldPos = model * glm::vec4(mv.pos, 1.0f);
```
currently has no check even though the `model` determinant is already being
checked a few lines earlier (for `normalMatrix`). Reuse that same guard:
```cpp
glm::vec4 worldPos = model * glm::vec4(mv.pos, 1.0f);
if (std::isnan(worldPos.x) || std::isnan(worldPos.y) || std::isnan(worldPos.z) ||
    !std::isfinite(worldPos.x) || !std::isfinite(worldPos.y) || !std::isfinite(worldPos.z)) {
    worldPos = glm::vec4(mv.pos, 1.0f); // fall back to local-space position rather than skip the vertex (keeps index buffer valid)
}
```
This is a last line of defense so that even if some *other*, not-yet-found
code path produces a bad `model` matrix in the future, it can never again
reach the GPU as a NaN vertex.

## Task 4: sanitize already-corrupted scenes on load

Because bad `scale`/`position`/`rotation` values may already be saved in
`.escn` files from before these guards existed, add a one-time sanitize
pass. In `include/SceneSerializer.h` (or wherever objects are finalized
after `Scene::Load`/deserialize), after populating each `GameObject`:
```cpp
auto sanitizeVec3 = [](glm::vec3& v, const glm::vec3& fallback) {
    if (std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.z) ||
        !std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z)) {
        v = fallback;
    }
};
sanitizeVec3(obj.position, glm::vec3(0.0f));
sanitizeVec3(obj.rotation, glm::vec3(0.0f));
sanitizeVec3(obj.scale, glm::vec3(1.0f));
obj.scale = glm::max(obj.scale, glm::vec3(1e-4f)); // no zero/negative axis on load either
```
Log a warning (`g_engineUI.AddLog("LogRecovery", ...)`) whenever a
sanitize actually changes a value, so it's visible which object in the
scene was corrupted and the user can re-check it.

## Acceptance criteria

- Selecting any object and dragging any gizmo (translate/rotate/scale),
  including objects parented under another object, never triggers
  `[RECOVERY] Present detected device loss (0x887a0006)`.
- Deliberately scaling an object to (or through) zero on an axis via the
  gizmo no longer corrupts `obj->scale` with NaN/zero — it clamps to a
  small positive minimum instead.
- Loading an old `.escn` file that has a corrupted (NaN/zero-scale) object
  from before this fix loads cleanly, logs a recovery warning naming the
  sanitized field, and does not hang on selection.