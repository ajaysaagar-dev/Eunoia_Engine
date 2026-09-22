cmake_minimum_required(VERSION 3.15)
project(Eunoia LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ──────────────────────────────────────────────────────────────────────────────
# Shared paths and dependencies
# ──────────────────────────────────────────────────────────────────────────────
set(EUNOIA_ROOT        ${CMAKE_CURRENT_SOURCE_DIR})
set(EUNOIA_DEPS        ${EUNOIA_ROOT}/deps)
set(EUNOIA_GLFW_ROOT   ${EUNOIA_DEPS}/glfw-3.5.1.bin.WIN64)

# ──────────────────────────────────────────────────────────────────────────────
# Strict Layered Engine Modules (Layer 1 -> Layer 6)
# Each layer depends ONLY on layers defined below it.
# ──────────────────────────────────────────────────────────────────────────────
# Layer 1: Zero dependencies, STL only
add_subdirectory(Engine/EngineCore)

# Layer 2: Windowing & Input (depends on Core)
add_subdirectory(Engine/EnginePlatform)

# Layer 3: Render Hardware Interface / D3D12 (depends on Core)
add_subdirectory(Engine/EngineRHI)

# Layer 4: Asset import/cook/load & Geometry (depends on RHI, Platform, Core)
add_subdirectory(Engine/EngineAssets)

# Layer 5: Render passes, materials, camera, lighting (depends on Assets, RHI, Core)
add_subdirectory(Engine/EngineRenderer)

# Layer 6: ECS, Scene graph, Behaviours, Serialization (depends on Renderer, Assets, Platform, Core)
add_subdirectory(Engine/EngineScene)

# Plugin Core System (depends on Core)
add_subdirectory(Engine/EunoiaPluginCore)

# ──────────────────────────────────────────────────────────────────────────────
# Application Targets
# ──────────────────────────────────────────────────────────────────────────────
# Layer 7: Editor executable with ImGui / ImGuizmo
add_subdirectory(Editor/Editor)

# Standalone Runtime executable (no ImGui dependency)
add_subdirectory(Runtime)

# ──────────────────────────────────────────────────────────────────────────────
# Test Targets (optional)
# ──────────────────────────────────────────────────────────────────────────────
option(EUNOIA_BUILD_TESTS "Build unit tests" ON)
if(EUNOIA_BUILD_TESTS)
    add_subdirectory(Tests)
endif()
