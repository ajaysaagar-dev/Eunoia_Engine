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
    ${EUNOIA_ROOT}/plugins/engine/include
    ${EUNOIA_ROOT}/plugins/platform/include
    ${EUNOIA_ROOT}/plugins/rhi/include
    ${EUNOIA_ROOT}/plugins/scene/include
    ${EUNOIA_ROOT}/plugins/rendering/include
    ${EUNOIA_ROOT}/plugins/primitives/include
    ${EUNOIA_ROOT}/plugins/assets/include
    ${EUNOIA_ROOT}/plugins/materials/include
    ${EUNOIA_ROOT}/plugins/levels/include
    ${EUNOIA_ROOT}/plugins/lights/include
    ${EUNOIA_ROOT}/plugins/behaviours/include
    ${EUNOIA_DEPS}/glm
    ${EUNOIA_DEPS}/json
)
target_link_libraries(PluginManagerTest PRIVATE EunoiaPluginCore EngineCore)
add_test(NAME PluginManagerTest COMMAND PluginManagerTest)
