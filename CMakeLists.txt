cmake_minimum_required(VERSION 3.15)
project(EunoiaEditor LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ──────────────────────────────────────────────────────────────────────────────
# Common include paths shared by every target
# ──────────────────────────────────────────────────────────────────────────────
set(EUNOIA_ROOT        ${CMAKE_CURRENT_SOURCE_DIR})
set(EUNOIA_DEPS        ${EUNOIA_ROOT}/deps)
set(EUNOIA_GLFW_ROOT   ${EUNOIA_DEPS}/glfw-3.5.1.bin.WIN64)

set(EUNOIA_COMMON_INCLUDES
    ${EUNOIA_ROOT}
    ${EUNOIA_ROOT}/include
    # Module include paths (for #include <EngineCore/Log.h> etc.)
    ${EUNOIA_ROOT}/EngineCore/include
    ${EUNOIA_ROOT}/EnginePlatform/include
    ${EUNOIA_ROOT}/EngineRHI/include
    ${EUNOIA_ROOT}/EngineRenderer/include
    ${EUNOIA_ROOT}/EngineScene/include
    ${EUNOIA_ROOT}/EngineAssets/include
    ${EUNOIA_ROOT}/Editor/include
    # Third-party
    ${EUNOIA_DEPS}/imgui
    ${EUNOIA_DEPS}/imgui/backends
    ${EUNOIA_GLFW_ROOT}/include
    ${EUNOIA_DEPS}/glm
    ${EUNOIA_DEPS}/tinyobj
    ${EUNOIA_DEPS}/cgltf
    ${EUNOIA_DEPS}/ufbx
)

# ──────────────────────────────────────────────────────────────────────────────
# EngineCore — no D3D12, no windowing
# ──────────────────────────────────────────────────────────────────────────────
# (Header-only for now; future: add EngineCore/src/*.cpp)
add_library(EngineCore INTERFACE)
target_include_directories(EngineCore INTERFACE
    ${EUNOIA_ROOT}/EngineCore/include
)

# ──────────────────────────────────────────────────────────────────────────────
# EnginePlatform — windowing + input (GLFW only)
# ──────────────────────────────────────────────────────────────────────────────
# (Header-only for now; future: move Window.cpp here)
add_library(EnginePlatform INTERFACE)
target_include_directories(EnginePlatform INTERFACE
    ${EUNOIA_ROOT}/EnginePlatform/include
    ${EUNOIA_GLFW_ROOT}/include
)
target_link_libraries(EnginePlatform INTERFACE EngineCore)

# ──────────────────────────────────────────────────────────────────────────────
# EngineScene — no rendering, no platform
# ──────────────────────────────────────────────────────────────────────────────
add_library(EngineScene INTERFACE)
target_include_directories(EngineScene INTERFACE
    ${EUNOIA_ROOT}/EngineScene/include
    ${EUNOIA_ROOT}/include          # GameObect.h, Scene.h etc. still here
    ${EUNOIA_DEPS}/glm
)
target_link_libraries(EngineScene INTERFACE EngineCore)

# ──────────────────────────────────────────────────────────────────────────────
# EngineAssets — depends on Core + Scene
# ──────────────────────────────────────────────────────────────────────────────
add_library(EngineAssets STATIC
    ${EUNOIA_ROOT}/src/AssetRegistry.cpp
    ${EUNOIA_ROOT}/src/AssetManager.cpp
    ${EUNOIA_ROOT}/src/MeshImporter.cpp
    ${EUNOIA_DEPS}/ufbx/ufbx.c
)
target_include_directories(EngineAssets PUBLIC
    ${EUNOIA_ROOT}/EngineAssets/include
    ${EUNOIA_COMMON_INCLUDES}
)
target_link_libraries(EngineAssets PUBLIC EngineScene EngineCore)

# ──────────────────────────────────────────────────────────────────────────────
# EngineRHI — the ONLY target that includes <d3d12.h>
# ──────────────────────────────────────────────────────────────────────────────
# (Header-only for now — D3D12 types used by EngineRHI headers flow through
#  to Cube.cpp which still owns the concrete implementations)
add_library(EngineRHI INTERFACE)
target_include_directories(EngineRHI INTERFACE
    ${EUNOIA_ROOT}/EngineRHI/include
)
target_link_libraries(EngineRHI INTERFACE EngineCore)

# ──────────────────────────────────────────────────────────────────────────────
# EngineRenderer — depends on RHI + Scene, no ImGui
# ──────────────────────────────────────────────────────────────────────────────
add_library(EngineRenderer INTERFACE)
target_include_directories(EngineRenderer INTERFACE
    ${EUNOIA_ROOT}/EngineRenderer/include
    ${EUNOIA_COMMON_INCLUDES}
)
target_link_libraries(EngineRenderer INTERFACE EngineRHI EngineScene EngineCore)

# ──────────────────────────────────────────────────────────────────────────────
# Editor executable — links everything + ImGui / ImGuizmo
# ──────────────────────────────────────────────────────────────────────────────
set(EDITOR_SOURCES
    ${EUNOIA_ROOT}/Cube.cpp
    ${EUNOIA_ROOT}/src/EngineUI.cpp
    ${EUNOIA_ROOT}/src/TextureManager.cpp
    # ImGui
    ${EUNOIA_DEPS}/imgui/imgui.cpp
    ${EUNOIA_DEPS}/imgui/imgui_draw.cpp
    ${EUNOIA_DEPS}/imgui/imgui_tables.cpp
    ${EUNOIA_DEPS}/imgui/imgui_widgets.cpp
    ${EUNOIA_DEPS}/imgui/backends/imgui_impl_glfw.cpp
    ${EUNOIA_DEPS}/imgui/backends/imgui_impl_dx12.cpp
    ${EUNOIA_DEPS}/imgui/ImGuizmo.cpp
)

add_executable(Eunoia-Editor ${EDITOR_SOURCES})
target_include_directories(Eunoia-Editor PRIVATE ${EUNOIA_COMMON_INCLUDES})
target_link_directories(Eunoia-Editor PRIVATE ${EUNOIA_GLFW_ROOT}/lib-mingw-w64)
target_link_libraries(Eunoia-Editor PRIVATE
    EngineRenderer
    EngineAssets
    EnginePlatform
    EngineCore
    glfw3
    d3d12
    dxgi
    d3dcompiler
    gdi32
    imm32
    comdlg32
)

# ──────────────────────────────────────────────────────────────────────────────
# Tests (optional, no GPU required)
# ──────────────────────────────────────────────────────────────────────────────
option(EUNOIA_BUILD_TESTS "Build unit tests" OFF)
if (EUNOIA_BUILD_TESTS)
    enable_testing()
    # Add test targets here when test files are created
    # add_subdirectory(tests/EngineCore.tests)
    # add_subdirectory(tests/EngineScene.tests)
    # add_subdirectory(tests/EngineAssets.tests)
endif()
