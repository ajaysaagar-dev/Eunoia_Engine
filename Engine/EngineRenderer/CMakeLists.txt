cmake_minimum_required(VERSION 3.15)

# Layer 5: EngineRenderer — render passes, materials, camera, lighting
add_library(EngineRenderer INTERFACE)

target_include_directories(EngineRenderer INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
    $<INSTALL_INTERFACE:Include>
    ${EUNOIA_DEPS}/glm
)

target_link_libraries(EngineRenderer INTERFACE
    EngineAssets
    EngineRHI
    EngineCore
)

target_compile_features(EngineRenderer INTERFACE cxx_std_17)
