cmake_minimum_required(VERSION 3.15)

# Thin Runtime target for game-only executable with NO Editor/ImGui dependency
add_executable(Eunoia-Runtime
    src/Main.cpp
)

target_include_directories(Eunoia-Runtime PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_directories(Eunoia-Runtime PRIVATE
    ${EUNOIA_GLFW_ROOT}/lib-mingw-w64
)

target_link_libraries(Eunoia-Runtime PRIVATE
    EngineScene
    EngineRenderer
    EngineAssets
    EngineRHI
    EnginePlatform
    EngineCore
    glfw3
    d3d12
    dxgi
    d3dcompiler
    gdi32
)

target_compile_features(Eunoia-Runtime PRIVATE cxx_std_17)
