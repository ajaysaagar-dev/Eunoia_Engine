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

# EngineScene Tests
add_executable(SceneTest
    EngineScene/SceneTest.cpp
)
target_link_libraries(SceneTest PRIVATE EngineScene EngineRenderer EngineAssets EnginePlatform EngineCore)
add_test(NAME SceneTest COMMAND SceneTest)

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
