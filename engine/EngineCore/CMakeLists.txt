cmake_minimum_required(VERSION 3.15)

# Layer 1: EngineCore — zero dependencies on anything else (STL only)
add_library(EngineCore INTERFACE)

target_include_directories(EngineCore INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(EngineCore INTERFACE cxx_std_17)
