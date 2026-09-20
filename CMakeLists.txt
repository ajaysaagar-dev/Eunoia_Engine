cmake_minimum_required(VERSION 3.15)
project(EunoiaEditor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/glfw-3.5.1.bin.WIN64/include
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/glm
)

link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/glfw-3.5.1.bin.WIN64/lib-mingw-w64
)

set(ENGINE_SOURCES
    Cube.cpp
    src/EngineUI.cpp
    deps/imgui/imgui.cpp
    deps/imgui/imgui_draw.cpp
    deps/imgui/imgui_tables.cpp
    deps/imgui/imgui_widgets.cpp
    deps/imgui/backends/imgui_impl_glfw.cpp
    deps/imgui/backends/imgui_impl_dx12.cpp
    deps/imgui/ImGuizmo.cpp
)

add_executable(Eunoia-Editor ${ENGINE_SOURCES})

target_link_libraries(Eunoia-Editor PRIVATE
    glfw3
    d3d12
    dxgi
    d3dcompiler
    gdi32
    imm32
)
