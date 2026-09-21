# Task: Convert Eunoia Engine to a Plugin-Based Architecture

## Context

Eunoia is a C++17 / DirectX 12 game engine + Dear ImGui level editor, built with
CMake (repo: `ajaysaagar-dev/Eunoia_Engine`). It's mid-refactor: `EngineCore`,
`EnginePlatform`, `EngineRHI`, `EngineRenderer`, `EngineScene`, `EngineAssets`,
`Editor` each already have their own `include/<Module>/` folder, but every one
of them is a CMake `INTERFACE`/`STATIC` library linked directly into a single
`Eunoia-Editor.exe`. There's no runtime module boundary — nothing can be
rebuilt or reloaded independently, and modules can freely reach into each
other's internals since they all compile into one binary. A chunk of logic
(D3D12 device setup, procedural mesh generation, app entry point) still lives
in the pre-refactor root-level `Cube.cpp`, `include/`, and `src/`.

**Goal:** turn this into a real plugin system. Every subsystem becomes a
self-contained folder under `/plugins/<category>/<name>/` with its own
manifest and build file. New plugins are drop-in — adding a folder with a
`plugin.json` and a `CMakeLists.txt` is enough for the build and the runtime
loader to pick it up, no other files need to change.

## Non-negotiable design constraints

1. **ABI boundary is C, not C++.** Every plugin is a separately-compiled DLL.
   C++ vtable layout and name mangling are not guaranteed compatible across
   DLLs, even from the same compiler version. Only plain structs and C
   function pointers may cross the boundary. Each plugin exports exactly
   three C functions: `Eunoia_GetPluginInfo`, `Eunoia_CreatePlugin`,
   `Eunoia_DestroyPlugin`.
2. **Cross-plugin calls go through a typed `ServiceRegistry` only**, keyed by
   string name (not `typeid`/RTTI — identity isn't guaranteed stable across
   DLLs either), never through a plugin including another plugin's internal
   headers.
3. **Dependency order is declared, not assumed.** Each `plugin.json` lists
   `dependencies`. The loader topologically sorts by that list and hard-fails
   on a missing dependency or a cycle — it never silently skips or guesses
   order.
4. **Two tiers, one folder layout.** `engine`, `platform`, and `rhi` hold live
   GPU/OS resources (window, D3D12 device, swapchain) that can't be swapped
   under a running app — they're marked `"core": true` in their manifest,
   loaded first, statically linked into the host executable, and never
   hot-reloaded. Every other plugin (`rendering`, `scene`, `levels`, `assets`,
   `primitives`, `materials`, `lights`, `behaviours`, `editor/*`) is
   dynamically loaded from a `.dll` and can be hot-reloaded during editor use.
   Both tiers use the identical manifest shape and `IPlugin` lifecycle — only
   the build/load mechanics differ.
5. **Lifecycle is two-phase.** `OnRegister(registry)` runs for every plugin,
   in dependency order, publishing what that plugin provides. Only after
   *every* plugin has registered does `OnInit()` run, also in dependency
   order — this is when a plugin is allowed to `Resolve()`/`Require()`
   something another plugin published. Shutdown reverses the order.

## Target folder layout

```
/plugins
  /engine        EngineCore: Log, JobSystem, Time, UUID, Result, Assert        [core]
  /platform      EnginePlatform: Window, InputSystem                          [core]
  /rhi           EngineRHI: Device, SwapChain, CommandContext, Fence,
                 DescriptorAllocator, PipelineState, UploadHeap               [core]
  /rendering     EngineRenderer: SceneRenderer, ConstantBuffers, TextureManager
                 (MaterialSystem moves out — see /materials)
  /scene         EngineScene: Scene, GameObject, Camera (live graph state only)
  /levels        Level, LevelSerializer, SceneSerializer (load/save/stream,
                 split out from live scene state)
  /assets        EngineAssets: AssetManager, AssetRegistry, AssetMetadata,
                 SoftAssetReference + /importers (Fbx, Gltf, Obj, Mesh)
  /primitives    Procedural mesh gen currently in Cube.cpp/EngineUI.cpp:
                 Cube, Plane, Sphere, Cylinder, Pyramid, Torus, Ground Grid
  /materials     MaterialSystem, pulled out of EngineRenderer
  /lights        New: point/directional/spot light components — currently
                 only implied by the editor's light.png icon, not a real system
  /behaviours    EunoiaBehaviour, BehaviourRegistry
  /editor
    /outliner
    /details
    /content-browser
    /viewport
    /world-settings
    /console
```

Every leaf folder follows the same shape:

```
/plugins/<category>/<name>/
  plugin.json
  CMakeLists.txt
  include/<Name>/*.h     (public headers other plugins may include)
  src/*.cpp
```

## Files to create

### 1. `EunoiaPluginCore/PluginAPI.h`

The ABI contract. Contents:

- `#include <cstdint>`, `EUNOIA_EXPORT` macro (`__declspec(dllexport)` on
  Windows / `__attribute__((visibility("default")))` elsewhere).
- `constexpr uint32_t kPluginAPIVersion = 1;` — bump whenever `IPlugin`,
  `ServiceRegistry`, or `EngineContext` change shape in a binary-incompatible
  way.
- `enum class PluginCategory { Engine, Platform, RHI, Rendering, Scene,
  Levels, Assets, Primitives, Materials, Lights, Behaviours, Editor, Custom };`
- POD `struct PluginInfo { const char* name; const char* version;
  PluginCategory category; uint32_t apiVersion;
  const char* const* dependencies; bool isCore; };`
- `using PluginGetInfoFn = const PluginInfo* (*)();`
  `using PluginCreateFn  = IPlugin* (*)(EngineContext*);`
  `using PluginDestroyFn = void (*)(IPlugin*);`
- `#define EUNOIA_DECLARE_PLUGIN(PluginClass, InfoPtr)` — expands to the three
  exported C functions (`Eunoia_GetPluginInfo`, `Eunoia_CreatePlugin`,
  `Eunoia_DestroyPlugin`) wrapping `PluginClass`'s constructor/destructor.

### 2. `EunoiaPluginCore/IPlugin.h`

- `class EngineContext { public: virtual ~EngineContext() = default;
  virtual ServiceRegistry& Services() = 0; };`
- `class IPlugin` with pure virtuals `OnRegister(ServiceRegistry&)` and
  `OnInit()`, plus virtual-with-default `WantsUpdate() const`,
  `OnUpdate(float)`, `OnShutdown()`, `SupportsHotReload() const`,
  `OnSerializeState(std::vector<uint8_t>&)`,
  `OnDeserializeState(const std::vector<uint8_t>&)`.

### 3. `EunoiaPluginCore/ServiceRegistry.h`

- `template<typename T> void Provide(const std::string& name, T* instance)`
- `template<typename T> T* Resolve(const std::string& name) const`
- `template<typename T> T& Require(const std::string& name) const` — throws
  `std::runtime_error` if missing.
- `bool Has(const std::string& name) const`
- Backing store: `std::unordered_map<std::string, void*>`.

### 4. `EunoiaPluginCore/PluginManager.h` / `.cpp`

- `PluginManifest` struct parsed from `plugin.json`
  (name/version/category/apiVersion/isCore/dependencies/provides/
  manifestPath/binaryPath).
- `LoadedPlugin` struct (manifest, module handle, destroy fn, `IPlugin*`).
- `PluginManager::LoadAll(root)`:
  1. Recursively find every `plugins/**/plugin.json`.
  2. Parse each with `nlohmann::json` (vendor `deps/json/json.hpp`, MIT,
     single header — this is the one new third-party dependency).
  3. Topologically sort by `dependencies`; throw on missing dependency or
     cycle.
  4. In sorted order: `LoadLibrary`/`dlopen` the binary next to the manifest,
     resolve the three exported symbols, verify `apiVersion ==
     kPluginAPIVersion`, construct the instance, call `OnRegister`.
  5. Then, in the same order, call `OnInit()` on every loaded plugin.
- `Update(float dt)` — calls `OnUpdate` on every plugin where
  `WantsUpdate()` is true.
- `ShutdownAll()` — reverse order: `OnShutdown()` → destroy → unload.
- `HotReload(name)` — refuses if the plugin is core or
  `SupportsHotReload()` is false; otherwise serialize state, shut down,
  unload, reload, re-init, deserialize state, and reinsert at the same
  position in the load order.

### 5. `cmake/EunoiaPlugin.cmake`

- `eunoia_add_plugin(NAME ... CATEGORY ... SOURCES ... DEPENDS ...)` —
  builds a `SHARED` target named `EunoiaPlugin_<NAME>`, links
  `EunoiaPluginCore` plus `DEPENDS`, sets output dir to
  `${CMAKE_BINARY_DIR}/plugins/<category>/<name>/`, and copies that plugin's
  `plugin.json` next to the built binary as a post-build step.
- `eunoia_discover_plugins(ROOT)` — globs every `ROOT/<category>/<name>/`
  that contains a `CMakeLists.txt` and calls `add_subdirectory()` on it.
  Called once, from the root `CMakeLists.txt`.

### 6. Root `CMakeLists.txt` (rewrite)

- Keep `EngineCore`/`EnginePlatform`/`EngineRHI` as `INTERFACE` libs, sourced
  from `plugins/engine|platform|rhi/include`, statically linked into
  `Eunoia-Editor` (core tier, per constraint 4).
- Add `EunoiaPluginCore` as a small `STATIC` lib (`PluginManager.cpp` +
  headers).
- Replace the old fixed `add_library(EngineRenderer ...)` /
  `add_library(EngineAssets ...)` block with a single
  `eunoia_discover_plugins(${EUNOIA_ROOT}/plugins)` call.
- `Eunoia-Editor` becomes a thin bootstrapper: create window/device (core
  tier), construct a `PluginManager`, call `LoadAll("plugins/")`, run the
  loop calling `PluginManager::Update(dt)`, call `ShutdownAll()` on exit. Move
  whatever's left of `Cube.cpp`'s app-entry responsibility into
  `Editor/src/EditorApp.cpp`.

## Migration order (do this incrementally, verify build after each step)

1. Add `EunoiaPluginCore` (files above) and `cmake/EunoiaPlugin.cmake`.
   Nothing else changes yet; confirm it compiles standalone.
2. Extract `/plugins/primitives` first — it's pure, stateless procedural mesh
   generation currently in `Cube.cpp`/`EngineUI.cpp`, so it's the lowest-risk
   proof of the pattern. Define `IPrimitiveFactory` (`CreateCube`,
   `CreatePlane`, `CreateSphere`, `CreateCylinder`, `CreatePyramid`,
   `CreateTorus`, `CreateGroundGrid`), move the bodies over, register it as
   `"IPrimitiveFactory"`, set `SupportsHotReload()` to `true`.
3. Pull `MaterialSystem` out of `EngineRenderer` into `/plugins/materials` —
   it's already a separate header, so this is close to a file move plus a
   manifest.
4. Move each `Editor/include/Editor/Panels/*.h` into its own
   `/plugins/editor/<panel-name>/`, replacing direct `Scene`/`GameObject`
   includes with `ServiceRegistry` lookups.
5. Split `EngineScene` (live graph: `Scene`, `GameObject`, `Camera`) from a
   new `/plugins/levels` (`Level`, `LevelSerializer`, `SceneSerializer`).
6. Convert `rendering`, `assets`, `behaviours` the same way. Keep `engine`,
   `platform`, `rhi` core-tier per constraint 4 — wrap them in manifests for
   uniformity but do not make them `SHARED`/hot-reloadable.
7. Add `/plugins/lights` as new work — no existing code to migrate, just the
   component types and a manifest declaring it depends on `scene` and
   `rendering`.
8. Delete the now-empty root-level `include/`, `src/`, and `Cube.cpp` once
   everything they contained has moved into a plugin.

## Acceptance criteria

- `cmake --build .` succeeds with zero changes to the root `CMakeLists.txt`
  after adding a brand-new folder under `/plugins/<category>/<name>/` with a
  valid manifest and `CMakeLists.txt`.
- Deleting a non-core plugin's folder and reconfiguring does not break the
  build of any plugin that doesn't depend on it.
- Starting the editor with a `plugins/<x>/plugin.json` that lists a
  nonexistent dependency fails fast with a clear error naming the missing
  dependency, instead of loading in the wrong order or silently continuing.
- `primitives` can be hot-reloaded from the editor (rebuild its DLL
  externally, trigger `PluginManager::HotReload("primitives")`) without
  restarting `Eunoia-Editor.exe` or losing scene state.
- No plugin's `.cpp` includes another plugin's `include/` headers directly —
  every cross-plugin call goes through `ServiceRegistry::Resolve`/`Require`.