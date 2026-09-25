cmake_minimum_required(VERSION 3.15)

# Tests for engine modules (no GPU required)
enable_testing()

# EngineCore Tests
add_executable(JobSystemTest
    EngineCore/JobSystemTest.cpp
)
target_link_libraries(JobSystemTest PRIVATE EngineCore)
add_test(NAME JobSystemTest COMMAND JobSystemTest)

# EngineAssets Tests
add_executable(AssetRegistryTest
    EngineAssets/AssetRegistryTest.cpp
)
target_link_libraries(AssetRegistryTest PRIVATE EngineAssets EngineCore)
add_test(NAME AssetRegistryTest COMMAND AssetRegistryTest)

add_executable(ProjectSetupTest
    EngineAssets/ProjectSetupTest.cpp
)
target_include_directories(ProjectSetupTest PRIVATE ${EUNOIA_DEPS}/json)
target_link_libraries(ProjectSetupTest PRIVATE EngineAssets EngineCore)
add_test(NAME ProjectSetupTest COMMAND ProjectSetupTest)

# EngineScene Tests
add_executable(SceneTest
    EngineScene/SceneTest.cpp
)
target_link_libraries(SceneTest PRIVATE EngineScene EngineRenderer EngineAssets EnginePlatform EngineCore)
add_test(NAME SceneTest COMMAND SceneTest)

# EngineRHI Tests
add_executable(RHITest
    EngineRHI/RHITest.cpp
    ${EUNOIA_ROOT}/Engine/EngineRHI/Src/DescriptorAllocator.cpp
    ${EUNOIA_ROOT}/Engine/EngineRHI/Src/Fence.cpp
    ${EUNOIA_ROOT}/Engine/EngineRHI/Src/PipelineState.cpp
)
target_link_libraries(RHITest PRIVATE EngineRHI EngineCore d3d12 dxgi d3dcompiler)
add_test(NAME RHITest COMMAND RHITest)

# PluginSystem Tests
add_executable(PluginManagerTest
    PluginSystem/PluginManagerTest.cpp
)
target_include_directories(PluginManagerTest PRIVATE
    ${EUNOIA_ROOT}/Plugins/engine/include
    ${EUNOIA_ROOT}/Plugins/platform/include
    ${EUNOIA_ROOT}/Plugins/rhi/include
    ${EUNOIA_ROOT}/Plugins/scene/include
    ${EUNOIA_ROOT}/Plugins/rendering/include
    ${EUNOIA_ROOT}/Plugins/primitives/include
    ${EUNOIA_ROOT}/Plugins/assets/include
    ${EUNOIA_ROOT}/Plugins/materials/include
    ${EUNOIA_ROOT}/Plugins/levels/include
    ${EUNOIA_ROOT}/Plugins/lights/include
    ${EUNOIA_ROOT}/Plugins/behaviours/include
    ${EUNOIA_DEPS}/glm
    ${EUNOIA_DEPS}/json
)
target_link_libraries(PluginManagerTest PRIVATE EunoiaPluginCore EngineCore)
add_test(NAME PluginManagerTest COMMAND PluginManagerTest)
