# Eunoia Engine — Primitives Plugin
# Procedural mesh generation (Cube, Sphere, Cylinder, etc.)

eunoia_add_plugin(
    NAME        primitives
    SOURCES     src/PrimitivesPlugin.cpp
    PLUGIN_DIR  ${CMAKE_CURRENT_SOURCE_DIR}
    DEPS        scene rendering
)
