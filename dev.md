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