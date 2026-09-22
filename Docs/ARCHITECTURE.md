# Eunoia Engine Architecture & Design Specification

## 1. Architectural Overview

Eunoia Engine is built upon a strict, one-directional layered architecture designed for modularity, clean separation of concerns, compile-time boundary enforcement, and future backend flexibility (e.g., Vulkan / Metal).

### Strict One-Directional Layering

```
  ┌─────────────────────────────────────────────────────────────┐
  │                     Application Layer                       │
  │   Editor/Editor (Editor Exe)   |   Runtime (Runtime Exe)    │
  └──────────────────────────────┬──────────────────────────────┘
                                 │
  ┌──────────────────────────────▼──────────────────────────────┐
  │                        EngineScene                          │
  │     ECS, Scene Graph, Gameplay Behaviours, Serialization    │
  └──────────────────────────────┬──────────────────────────────┘
                                 │
  ┌──────────────────────────────▼──────────────────────────────┐
  │                       EngineRenderer                        │
  │     Render passes, Materials, Cameras, Lighting passes      │
  └──────────────────────────────┬──────────────────────────────┘
                                 │
  ┌──────────────────────────────▼──────────────────────────────┐
  │                        EngineAssets                         │
  │   Asset Registry, Asset Manager, Importers, Mesh/Texture    │
  └──────────────────────────────┬──────────────────────────────┘
                                 │
  ┌──────────────────────────────▼──────────────────────────────┐
  │                         EngineRHI                           │
  │    Render Hardware Interface (DirectX 12 API Abstraction)   │
  └──────────────────────────────┬──────────────────────────────┘
                                 │
  ┌──────────────────────────────▼──────────────────────────────┐
  │                       EnginePlatform                        │
  │     Windowing (GLFW), Mouse/Keyboard Input, OS Abstraction  │
  └──────────────────────────────┬──────────────────────────────┘
                                 │
  ┌──────────────────────────────▼──────────────────────────────┐
  │                         EngineCore                          │
  │   Job System, Logging, Time, UUID, Assertions (STL only)    │
  └─────────────────────────────────────────────────────────────┘
```

> [!IMPORTANT]
> **Strict Layering Invariant**: A module may **only** depend on modules listed below it in the diagram. Linking or including upward is strictly prohibited.

---

## 2. Layer Definitions & Responsibilities

### Layer 1: `EngineCore`
- **Location**: `Engine/EngineCore`
- **Dependencies**: **Zero** external dependencies. Standard C++ Library (STL) only.
- **Responsibilities**:
  - Thread pools and multi-threaded job scheduling (`JobSystem.h`)
  - Logging and diagnostics (`EngineLogger.h`, `Log.h`)
  - High-precision time and delta measurement (`Time.h`)
  - Globally unique identifier generation (`UUID.h`)
  - Core assertions, error tracking, and result wrappers (`Assert.h`, `Result.h`)

### Layer 2: `EnginePlatform`
- **Location**: `Engine/EnginePlatform`
- **Dependencies**: `EngineCore`, GLFW3.
- **Responsibilities**:
  - Operating system window creation, sizing, and event handling (`Window.h`, `Window.cpp`)
  - Hardware mouse and keyboard input state abstraction (`InputSystem.h`)
  - Platform-specific message pump management

### Layer 3: `EngineRHI`
- **Location**: `Engine/EngineRHI`
- **Dependencies**: `EngineCore`, Direct3D 12 (`d3d12.h`, `dxgi1_6.h`, `d3dcompiler.h`).
- **Responsibilities**:
  - Render Hardware Interface wrapping all low-level GPU hardware constructs
  - Device initialization and adapter management (`Device.h`)
  - Swap chain creation and present logic (`SwapChain.h`)
  - Command lists and command allocator execution contexts (`CommandContext.h`)
  - CPU/GPU synchronization fences (`Fence.h`)
  - CPU/GPU descriptor allocators (`DescriptorAllocator.h`)
  - Dynamic GPU upload heaps (`UploadHeap.h`)
  - Graphics and compute pipeline state objects (`PipelineState.h`)
  > [!NOTE]
  > EngineRHI is the **only** module that interacts directly with DirectX 12 headers.

### Layer 4: `EngineAssets`
- **Location**: `Engine/EngineAssets`
- **Dependencies**: `EngineRHI`, `EnginePlatform`, `EngineCore`, GLM, third-party loaders (`cgltf`, `tinyobj`, `ufbx`, `StbImage`).
- **Responsibilities**:
  - Asset metadata, virtual path system, and unique Asset IDs (`AssetID.h`, `AssetPath.h`, `AssetMetadata.h`)
  - Thread-safe Asset Registry indexing disk assets and `.assetmeta` files (`AssetRegistry.h`, `AssetRegistry.cpp`)
  - Asset Manager with reference caching, streaming, and soft references (`AssetManager.h`, `AssetManager.cpp`, `SoftAssetReference.h`)
  - 3D model loaders for FBX, OBJ, and GLTF formats (`MeshImporter.h`, `MeshImporter.cpp`)
  - Texture decoding, path resolution, and bilinear CPU sampling (`TextureManager.h`, `TextureManager.cpp`)
  - Vertex layout and primitive geometry generation (`Geometry.h`)

### Layer 5: `EngineRenderer`
- **Location**: `Engine/EngineRenderer`
- **Dependencies**: `EngineAssets`, `EngineRHI`, `EngineCore`, GLM.
- **Responsibilities**:
  - Camera projections, view matrices, and orbit/flying controls (`Camera.h`)
  - Material representations and shader binding definitions (`MaterialSystem.h`)
  - GPU constant buffer structures (`ConstantBuffers.h`)
  - High-level scene rendering passes: shadow mapping, forward/deferred PBR passes (`SceneRenderer.h`)
  > [!NOTE]
  > EngineRenderer communicates with the GPU strictly via EngineRHI abstractions, never via raw D3D12 calls.

### Layer 6: `EngineScene`
- **Location**: `Engine/EngineScene`
- **Dependencies**: `EngineRenderer`, `EngineAssets`, `EnginePlatform`, `EngineCore`, GLM.
- **Responsibilities**:
  - World representation and actor hierarchy (`Scene.h`, `Level.h`)
  - Entity transform hierarchies, mesh associations, and light definitions (`GameObject.h`)
  - JSON scene serialization, level persistence, and asset binding (`SceneSerializer.h`, `LevelSerializer.h`)
  - Extensible gameplay scripting system (`EunoiaBehaviour.h`, `EunoiaBehaviour.cpp`, `BehaviourRegistry.h`)
  - In-game debug HUD printing (`ScreenPrint.h`)

### Plugin System: `EunoiaPluginCore`
- **Location**: `Engine/EunoiaPluginCore`
- **Dependencies**: `EngineCore`.
- **Responsibilities**:
  - Dynamic library plugin loader and lifecycle manager (`PluginManager.h`, `PluginManager.cpp`)
  - Service registry for decoupling subsystems (`ServiceRegistry.h`)
  - Plugin extension API interfaces (`IPlugin.h`, `PluginAPI.h`, `IEditorPanel.h`)

### Layer 7 (Application Targets):
#### Editor: `Editor/Editor`
- **Location**: `Editor/Editor`
- **Dependencies**: `EngineScene`, `EngineRenderer`, `EngineAssets`, `EngineRHI`, `EnginePlatform`, `EngineCore`, `EunoiaPluginCore`, Dear ImGui, ImGuizmo.
- **Responsibilities**:
  - Full editor UI suite (`EngineUI.h`, `EngineUI.cpp`)
  - Dockable panels: Outliner, Details, Viewport, Content Browser, Console, World Settings (`Panels/*.h`)
  - 52-step Undo/Redo system (`UndoManager.h`)
  - Editor main entry point and render loop (`Main.cpp`, `EditorApp.h`)

#### Standalone Runtime: `Runtime`
- **Location**: `Runtime`
- **Dependencies**: `EngineScene`, `EngineRenderer`, `EngineAssets`, `EngineRHI`, `EnginePlatform`, `EngineCore`. **Zero ImGui dependency.**
- **Responsibilities**:
  - Lightweight game-only client execution (`Src/Main.cpp`)
  - Direct scene execution without editor overlays or UI overhead

---

## 3. Directory Layout

```
C:\Projects\Eunoia-Engine
├── .github/
│   └── workflows/
│       └── ci.yml                 # Automated build and test workflow
├── CMake/                         # Shared CMake helper modules
├── Docs/
│   ├── ARCHITECTURE.md            # Architectural specification (this document)
│   ├── Dev.md                     # Refactoring specification and roadmap
│   └── UI_Ref.svg                 # UI/UX reference design
├── Engine/
│   ├── EngineCore/                # Layer 1: Core primitives (STL only)
│   ├── EnginePlatform/            # Layer 2: Platform & Windowing (GLFW)
│   ├── EngineRHI/                 # Layer 3: DirectX 12 hardware abstraction
│   ├── EngineAssets/              # Layer 4: Asset system, loaders & geometry
│   ├── EngineRenderer/            # Layer 5: Renderer, cameras & materials
│   ├── EngineScene/               # Layer 6: Scene, game objects & behaviours
│   └── EunoiaPluginCore/          # Modular plugin manager & service registry
├── Editor/
│   └── Editor/                    # Layer 7: Editor UI, Panels, ImGui
├── Runtime/                       # Standalone headless/game runtime stub
├── Tests/                         # Unit tests per engine module
├── Deps/                          # External libraries (glm, glfw, imgui, ufbx, etc.)
├── Projects/                      # User and sample projects
│   └── <ProjectName>/
│       ├── Behaviours/
│       ├── Cooked/                # Cooked content for the project
│       │   ├── Content/           # Runtime-optimized assets
│       │   └── CookedAssetRegistry.json
│       ├── Levels/
│       ├── Materials/
│       ├── Models/
│       └── Scenes/
├── Resources/                     # Editor icons, textures, default assets
│   └── Icons/
├── Shaders/                       # HLSL DirectX 12 shaders
├── Tools/                         # Toolchain binaries and scripts
├── CMakeLists.txt                 # Master CMake build configuration
├── Build.bat                      # Primary compilation & packaging script
└── Run.bat                        # Engine launcher script
```

---

## 4. Cooked Content Architecture

All asset cooking operations store output strictly within the respective game project's folder:

```
Projects/<GameProject>/Cooked/
├── Content/
│   ├── <AssetUUID>.glb
│   ├── <AssetUUID>.png
│   └── ...
└── CookedAssetRegistry.json
```

- Cooking never dumps artifacts to the engine root.
- The default target directory is resolved relative to the active project's root: `<ProjectRoot>/Cooked`.

---

## 5. Include and Module Rules

1. **Header Inclusion Standard**:
   All inter-module inclusions must specify the module prefix:
   ```cpp
   #include <EngineCore/Log.h>
   #include <EnginePlatform/InputSystem.h>
   #include <EngineAssets/Geometry.h>
   #include <EngineRenderer/Camera.h>
   #include <EngineScene/Scene.h>
   #include <Editor/EngineUI.h>
   ```

2. **Public vs. Private Separation**:
   - Public APIs live in `Include/<ModuleName>/`.
   - Implementation files live in `Src/`.

3. **No Direct GPU Calls Outside RHI**:
   Rendering logic in `EngineRenderer` and scene manipulation in `EngineScene` interact with GPU resources through `EngineRHI` abstraction types.
