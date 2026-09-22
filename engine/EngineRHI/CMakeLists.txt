cmake_minimum_required(VERSION 3.15)

# Layer 3: EngineRHI — render hardware interface (D3D12 abstraction)
# Wraps ID3D12Device, command lists, descriptor heaps, fences.
add_library(EngineRHI INTERFACE)

target_include_directories(EngineRHI INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(EngineRHI INTERFACE
    EngineCore
    d3d12
    dxgi
    d3dcompiler
)

target_compile_features(EngineRHI INTERFACE cxx_std_17)
