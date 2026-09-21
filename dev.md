# Task: Finish the Plugin Migration and Close the Engine's Feature Gaps

## Context

Eunoia is a C++17 / DirectX 12 engine + Dear ImGui editor. Three commits
(`732a1d8`, `518e9e7`, `c847172`) added a `/plugins/<category>/<name>/`
architecture (manifests, `EunoiaPluginCore`, per-plugin `.cpp` stubs for
`engine`, `platform`, `rhi`, `rendering`, `scene`, `levels`, `assets`,
`primitives`, `materials`, `lights`, `behaviours`, and six `editor/*` panels)
alongside `tests/PluginSystem.tests/PluginManagerTest.cpp`.

**None of this is connected to the real build.** Root `CMakeLists.txt` still
compiles the pre-plugin layout (`Cube.cpp`, `EngineRenderer/`, `EngineAssets/`,
etc. at repo root) directly into `Eunoia-Editor.exe`. It never includes
`cmake/EunoiaPlugin.cmake`, never calls `eunoia_discover_plugins()`, never
links `EunoiaPluginCore`, and nothing in `main()`/`Cube.cpp` ever constructs a
`PluginManager` or calls `LoadAll()`. Only `plugins/primitives/CMakeLists.txt`
exists — every other plugin folder has no build file at all. The test target
isn't wired into `CMakeLists.txt` either (`EUNOIA_BUILD_TESTS` block has the
`add_subdirectory` calls commented out).

There are also three overlapping copies of engine code right now:
`include/`+`src/` (pre-refactor, e.g. `include/Scene.h`, `include/GameObject.h`,
`include/EunoiaBehaviour.h`), `EngineCore/`/`EngineRenderer/`/`EngineScene/`/
etc. at repo root (the first module-split attempt, mostly thin forwarding
headers), and `/plugins/*` (the newest split, currently disconnected). This
must be reduced to one before any further feature work, or every change has
to be made three times.

Work through the sections below **in order** — later sections assume earlier
ones are done and building.

---

## Phase 1 (P0) — Make the plugin system real

This phase produces zero new user-facing features. Its only job is to make
the three existing plugin commits actually run, and collapse the three
parallel copies of the engine into one.

1. **Give every plugin folder a `CMakeLists.txt`.** Only `plugins/primitives`
   has one. Add one to `assets`, `behaviours`, `engine`, `levels`, `lights`,
   `materials`, `platform`, `rendering`, `rhi`, `scene`, and each
   `editor/<panel-name>` folder, each calling `eunoia_add_plugin(NAME ...
   CATEGORY ... SOURCES ... DEPENDS ...)` per the pattern already established
   in `plugins/primitives/CMakeLists.txt` and `cmake/EunoiaPlugin.cmake`.
2. **Wire discovery into the root build.** Add
   `include(${EUNOIA_ROOT}/cmake/EunoiaPlugin.cmake)` and
   `eunoia_discover_plugins(${EUNOIA_ROOT}/plugins)` to `CMakeLists.txt`.
   Add `EunoiaPluginCore` as a `STATIC` library
   (`EunoiaPluginCore/PluginManager.cpp` + headers) and link it into
   `Eunoia-Editor`.
3. **Load plugins at startup.** In `Cube.cpp` (or wherever `main()` lives),
   after the window/D3D12 device exist, construct a `PluginManager`, call
   `LoadAll("plugins")`, drive `Update(dt)` from the frame loop, and call
   `ShutdownAll()` on exit — matching the lifecycle already defined in
   `EunoiaPluginCore/PluginManager.h`.
4. **Wire the test target.** Uncomment/add the `add_subdirectory` calls for
   `tests/PluginSystem.tests`, `tests/EngineCore.tests`,
   `tests/EngineScene.tests`, `tests/EngineAssets.tests` under the existing
   `EUNOIA_BUILD_TESTS` option, and confirm
   `tests/PluginSystem.tests/PluginManagerTest.cpp` passes — it already
   asserts `LoadAll()` succeeds and prints topological load order, so a
   green run of that binary is your signal Phase 1 is done.
5. **Collapse the three engine copies into one.** For each subsystem, decide
   whether the `/plugins/<category>/` version or the legacy
   `include/`+`src/`/root-module version is the one moving forward (default:
   the plugin version, since that's where new work goes), move any real
   implementation that's still only in the legacy copy over to it, update
   every `#include` across the codebase accordingly, and delete the losing
   copy. Do this one subsystem at a time, confirming the build still succeeds
   after each, rather than as one giant sweep — a broken intermediate state
   is fine as long as it's a single subsystem's worth of `#include` churn.

**Acceptance for Phase 1:** `cmake --build .` succeeds; `Eunoia-Editor.exe`
starts, and its log output shows every plugin loading in dependency order
with no missing-dependency or api-version errors; `PluginManagerTest`
executable runs green; no header exists in more than one of
`include/`, `<Module>/include/`, `plugins/<category>/include/` for the same
subsystem.

---

## Phase 2 (P1) — Core gameplay loop

### 2a. Script authoring + compile pipeline

The engine already has a real foundation for this: `include/EunoiaBehaviour.h`
(lifecycle: `OnCreate`/`OnEnable`/`Start`/`Update`/`FixedUpdate`/`LateUpdate`/
`OnDisable`/`OnDestroy`, typed property reflection via `RegisterProperty`,
object-reference resolution, `GetComponent<T>`/`GetBehaviour<T>`) and
`include/BehaviourRegistry.h` (factory-based registration, five built-ins:
`RotatorBehaviour`, `LightFlickerBehaviour`, `DoorController`,
`PlayerController`, `EnemyController`). What's missing is everything the
former `dev.md` (recoverable at `git show 518e9e7:dev.md`) asked for around
*authoring* a behaviour from inside the editor:

- Content Browser → "Create → Behaviour" action that generates a `.h`/`.cpp`
  pair from a template, using `EunoiaBehaviour` as the base class and
  `REGISTER_BEHAVIOUR(ClassName, DisplayName)` already defined at the bottom
  of `BehaviourRegistry.h`.
- A build step that compiles just the new/changed behaviour source(s) into a
  loadable module (this is exactly what the Phase 1 plugin loader/hot-reload
  path exists for — a user behaviour is the natural case for
  `IPlugin::SupportsHotReload()` and `PluginManager::HotReload()`) and
  re-registers it with `BehaviourRegistry::Get()` without restarting the
  editor.
- Surface compile errors in the Console panel (`plugins/editor/console`)
  instead of failing silently.

### 2b. Physics / collision

There is currently no physics system anywhere in the codebase — not even a
stub. Add:

- A minimal collision layer first: AABB/sphere colliders attachable to
  `GameObject`, broad-phase + narrow-phase collision detection, and
  `OnCollisionEnter`/`OnCollisionStay`/`OnCollisionExit` callbacks threaded
  through `EunoiaBehaviour` alongside the existing lifecycle methods.
- Basic rigidbody dynamics (gravity, velocity integration, restitution)
  gated behind Play Mode (`Scene::isPlayMode`, `include/Scene.h`) only — the
  editor's static viewport shouldn't simulate physics while not playing.
- Register the system as `/plugins/physics` (new category, not in the
  original list — add it to `PluginCategory` in `EunoiaPluginCore/PluginAPI.h`)
  depending on `engine` and `scene`.

### 2c. Audio

`AssetType::Audio` (`include/AssetType.h`) already detects `.wav`/`.mp3`/
`.ogg` and has an icon, but there's no playback system behind it. Add a
`/plugins/audio` plugin providing an `IAudioSystem` (`PlaySound`,
`PlayMusic`, `Stop`, per-source volume/pitch/loop, 3D positional attenuation
tied to `GameObject` transforms) and an `AudioSourceBehaviour`/component so
designers can attach a sound to an object the same way they attach a
`Light` or `PrimitiveMesh` today.

**Acceptance for Phase 2:** a user can, without touching engine source,
create a new behaviour from the Content Browser, have it compile and attach
to a `GameObject`, drop a collider + rigidbody on two objects and see them
collide in Play Mode, and attach a looping sound to an object and hear it
play in Play Mode.

---

## Phase 3 (P2) — Content and rendering depth

1. **Verify material/asset round-tripping.** `EngineRenderer/include/
   EngineRenderer/MaterialSystem.h`, `include/MeshImporter.h`, and the
   Fbx/Gltf/Obj importer headers under `EngineAssets/include/EngineAssets/
   Importers/` exist; confirm a material created and assigned in the editor
   actually survives a full save (`include/SceneSerializer.h`,
   `include/LevelSerializer.h`) → close editor → reopen → reload cycle, not
   just an in-session assignment. Fix whatever breaks.
2. **Prefabs.** `AssetType::Prefab` exists as an enum value only. Add a real
   prefab asset (serialized `GameObject` subtree + component/behaviour data),
   an instancing path from the Content Browser into the scene, and
   override-tracking so an instance can diverge from its source prefab.
3. **Animation / skeletal mesh.** `AssetType::Animation` and
   `AssetType::SkeletalMesh` are enum-only. Scope this as its own follow-up
   task once Phases 1–2 land — needs skeleton/bone data in the mesh import
   path (`ufbx`/`cgltf` already vendored under `deps/`), skinning in the
   vertex shader (see the metallic/roughness PBR shader inlined in
   `Cube.cpp` around line 1358 for the current shader-embedding pattern), and
   a timeline/clip playback system. Don't start it until Phase 2 is solid.
4. **Shadow parity.** Point-light shadows were enabled in `2ac1d0c`; bring
   directional and spot lights to the same coverage, and add basic shadow
   quality/resolution settings to World Settings
   (`plugins/editor/world-settings`).

---

## Phase 4 (P3) — Production and polish

1. **Make hot reload real.** Once Phase 1 lands, exercise
   `PluginManager::HotReload()` on a genuinely stateless plugin first
   (`primitives` is the right candidate — see its
   `SupportsHotReload() { return true; }`), confirm it round-trips cleanly,
   then extend the same path to user behaviours from Phase 2a.
2. **Standalone runtime export.** `run.bat`/`build.bat` currently only build
   the editor. Add a packaging step that bundles a built level + only the
   plugins/assets it references into a runnable, editor-free executable.
3. **Particle system.** `AssetType::Particle` is enum-only; lowest priority
   of the remaining stub systems — schedule after Phase 3.

---

## Ground rules for every phase

- Confirm the build after each individual change, not at the end of a phase.
  A phase that leaves the tree non-building for CMake is not done.
- Reuse what's already there. `UndoManager` (`include/UndoManager.h`, 52-step
  snapshot undo/redo), Play Mode (`Scene::StartPlayMode`/`StopPlayMode`,
  `include/Scene.h`), and the existing `BehaviourRegistry`/`EunoiaBehaviour`
  reflection system are all real, working features — extend them, don't
  replace them.
- New cross-plugin functionality goes through `ServiceRegistry`
  (`EunoiaPluginCore/ServiceRegistry.h`), never a direct include of another
  plugin's internal headers — this is the whole point of Phase 1.
- Update `plugin.json` `dependencies`/`provides` for every plugin you touch
  so `PluginManager`'s topological sort and the manifest stay truthful.