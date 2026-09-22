cmake_minimum_required(VERSION 3.15)

# Layer 6: EngineScene — ECS/scene graph, gameplay-facing API, Behaviours
add_library(EngineScene STATIC
    Src/EunoiaBehaviour.cpp
)

target_include_directories(EngineScene PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
    $<INSTALL_INTERFACE:Include>
    ${EUNOIA_DEPS}/glm
)

target_link_libraries(EngineScene PUBLIC
    EngineRenderer
    EngineAssets
    EnginePlatform
    EngineCore
)

target_compile_features(EngineScene PUBLIC cxx_std_17)
