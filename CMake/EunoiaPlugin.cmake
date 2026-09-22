# ============================================================================
# Eunoia Engine — Plugin CMake Helpers
# ============================================================================
# eunoia_add_plugin()         — declare a single plugin target
# eunoia_discover_plugins()   — auto-scan a directory for plugins
# ============================================================================

# ---------------------------------------------------------------------------
# eunoia_add_plugin(
#   NAME        <plugin-name>
#   SOURCES     <source files...>
#   DEPS        <cmake target dependencies...>
#   CORE        <ON|OFF>          # default OFF
#   PLUGIN_DIR  <path to plugin>  # for locating plugin.json
# )
#
# Creates either a STATIC library (core) or SHARED library (dynamic).
# Automatically links EunoiaPluginCore and sets up include paths.
# ---------------------------------------------------------------------------
function(eunoia_add_plugin)
    cmake_parse_arguments(
        PLUGIN                           # prefix
        "CORE"                           # boolean options
        "NAME;PLUGIN_DIR"                # single-value args
        "SOURCES;DEPS"                   # multi-value args
        ${ARGN}
    )

    if(NOT PLUGIN_NAME)
        message(FATAL_ERROR "eunoia_add_plugin: NAME is required")
    endif()

    if(PLUGIN_CORE)
        # Core plugins are static libraries linked into the host
        add_library(${PLUGIN_NAME} STATIC ${PLUGIN_SOURCES})
        target_compile_definitions(${PLUGIN_NAME} PRIVATE EUNOIA_PLUGIN_STATIC)
    else()
        # Dynamic plugins are shared libraries (DLLs)
        add_library(${PLUGIN_NAME} SHARED ${PLUGIN_SOURCES})
        target_compile_definitions(${PLUGIN_NAME} PRIVATE EUNOIA_PLUGIN_EXPORT)
        
        # Output DLL next to the host executable
        set_target_properties(${PLUGIN_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
        )
    endif()

    # Common setup
    target_include_directories(${PLUGIN_NAME} PUBLIC
        ${PLUGIN_PLUGIN_DIR}/include
    )
    target_include_directories(${PLUGIN_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/EunoiaPluginCore
        ${CMAKE_SOURCE_DIR}/deps/json
    )
    target_link_libraries(${PLUGIN_NAME} PRIVATE EunoiaPluginCore)

    # Link declared dependencies
    if(PLUGIN_DEPS)
        target_link_libraries(${PLUGIN_NAME} PRIVATE ${PLUGIN_DEPS})
    endif()

    # C++17
    target_compile_features(${PLUGIN_NAME} PRIVATE cxx_std_17)

    message(STATUS "[Eunoia] Plugin: ${PLUGIN_NAME} (${PLUGIN_CORE}core)")
endfunction()


# ---------------------------------------------------------------------------
# eunoia_discover_plugins(DIR)
#
# Recursively scan DIR for plugin.json files.  For each one found, parse
# the manifest and call eunoia_add_plugin() with the appropriate settings.
#
# Each plugin directory is expected to have:
#   plugin.json
#   CMakeLists.txt   (optional — if present, add_subdirectory is used instead)
#   src/*.cpp
#   include/<Name>/*.h
# ---------------------------------------------------------------------------
function(eunoia_discover_plugins PLUGINS_DIR)
    file(GLOB_RECURSE MANIFEST_FILES "${PLUGINS_DIR}/**/plugin.json")

    foreach(MANIFEST ${MANIFEST_FILES})
        get_filename_component(PLUGIN_DIR ${MANIFEST} DIRECTORY)
        get_filename_component(PLUGIN_FOLDER_NAME ${PLUGIN_DIR} NAME)

        # If the plugin has its own CMakeLists.txt, defer to it
        if(EXISTS "${PLUGIN_DIR}/CMakeLists.txt")
            add_subdirectory(${PLUGIN_DIR})
            message(STATUS "[Eunoia] Discovered plugin (custom build): ${PLUGIN_FOLDER_NAME}")
        else()
            # Auto-discover sources
            file(GLOB_RECURSE PLUGIN_SOURCES "${PLUGIN_DIR}/src/*.cpp" "${PLUGIN_DIR}/src/*.c")
            
            if(PLUGIN_SOURCES)
                # Read manifest to determine if core
                file(READ ${MANIFEST} MANIFEST_CONTENT)
                string(FIND "${MANIFEST_CONTENT}" "\"core\": true" CORE_POS)

                if(CORE_POS GREATER -1)
                    eunoia_add_plugin(
                        NAME ${PLUGIN_FOLDER_NAME}
                        SOURCES ${PLUGIN_SOURCES}
                        PLUGIN_DIR ${PLUGIN_DIR}
                        CORE
                    )
                else()
                    eunoia_add_plugin(
                        NAME ${PLUGIN_FOLDER_NAME}
                        SOURCES ${PLUGIN_SOURCES}
                        PLUGIN_DIR ${PLUGIN_DIR}
                    )
                endif()
            else()
                message(STATUS "[Eunoia] Discovered plugin (no sources yet): ${PLUGIN_FOLDER_NAME}")
            endif()
        endif()
    endforeach()
endfunction()
