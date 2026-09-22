cmake_minimum_required(VERSION 3.15)

# Layer 2: EnginePlatform — windowing, input, OS abstraction (GLFW only)
add_library(EnginePlatform STATIC
    src/Window.cpp
)

target_include_directories(EnginePlatform PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(EnginePlatform PRIVATE
    ${EUNOIA_GLFW_ROOT}/include
)

target_link_libraries(EnginePlatform PUBLIC
    EngineCore
)

target_link_libraries(EnginePlatform PRIVATE
    glfw3
)

target_compile_features(EnginePlatform PUBLIC cxx_std_17)
