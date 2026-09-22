# ============================================================================
# Eunoia Engine — Mesh Cluster Culling Plugin CMake
# ============================================================================

set(CLUSTER_CULLING_SOURCES
    Source/MeshClusterBuilder.cpp
    Source/MeshClusterCulling.cpp
    Source/HiZManager.cpp
)

eunoia_add_plugin(
    NAME        Mesh-Cluster-Culling
    SOURCES     ${CLUSTER_CULLING_SOURCES}
    PLUGIN_DIR  ${CMAKE_CURRENT_SOURCE_DIR}
    CORE
    DEPS        EngineCore EnginePlatform EngineRHI EngineAssets EngineRenderer EngineScene
)
