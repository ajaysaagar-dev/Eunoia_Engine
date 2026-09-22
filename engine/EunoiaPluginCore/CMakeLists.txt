cmake_minimum_required(VERSION 3.15)

# Plugin core system
add_library(EunoiaPluginCore STATIC
    src/PluginManager.cpp
)

target_include_directories(EunoiaPluginCore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(EunoiaPluginCore PUBLIC
    EngineCore
)

target_compile_features(EunoiaPluginCore PUBLIC cxx_std_17)
