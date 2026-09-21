#pragma once
// ============================================================================
// Eunoia Engine — Plugin API (C ABI Boundary)
// ============================================================================
// This header defines the stable C ABI contract that every plugin DLL must
// export.  Only plain structs and C function pointers cross the DLL boundary.
// C++ vtable layout and name mangling aren't guaranteed compatible across
// separately-compiled DLLs, so we use extern "C" exports exclusively.
//
// Use the EUNOIA_DECLARE_PLUGIN(ClassName) macro at the bottom of your
// plugin's main .cpp to generate the three required exports automatically.
// ============================================================================

#include <cstdint>

// ---------------------------------------------------------------------------
// DLL export / import macros
// ---------------------------------------------------------------------------
#ifdef _WIN32
    #ifdef EUNOIA_PLUGIN_STATIC
        #define EUNOIA_API extern "C"
    #elif defined(EUNOIA_PLUGIN_EXPORT)
        #define EUNOIA_API extern "C" __declspec(dllexport)
    #else
        #define EUNOIA_API extern "C" __declspec(dllimport)
    #endif
#else
    #define EUNOIA_API extern "C" __attribute__((visibility("default")))
#endif

// ---------------------------------------------------------------------------
// Plugin API version — bump when IPlugin or ServiceRegistry change shape
// ---------------------------------------------------------------------------
constexpr uint32_t EUNOIA_PLUGIN_API_VERSION = 1;

// ---------------------------------------------------------------------------
// Plugin category enum (matches folder names under /plugins/)
// ---------------------------------------------------------------------------
enum class EunoiaPluginCategory : uint32_t {
    Engine      = 0,
    Platform    = 1,
    RHI         = 2,
    Rendering   = 3,
    Scene       = 4,
    Levels      = 5,
    Assets      = 6,
    Primitives  = 7,
    Materials   = 8,
    Lights      = 9,
    Behaviours  = 10,
    Editor      = 11,
    Custom      = 255
};

// ---------------------------------------------------------------------------
// PluginInfo — POD struct returned by Eunoia_GetPluginInfo()
// ---------------------------------------------------------------------------
struct EunoiaPluginInfo {
    const char*          name;           // Unique identifier (matches plugin.json "name")
    const char*          version;        // Semantic version string "major.minor.patch"
    const char*          description;    // Human-readable description
    const char*          author;         // Author / team name
    uint32_t             apiVersion;     // Must equal EUNOIA_PLUGIN_API_VERSION
    EunoiaPluginCategory category;       // Which tier / folder
    bool                 isCore;         // true = statically linked, never hot-reloaded
};

// ---------------------------------------------------------------------------
// Forward declarations — defined in IPlugin.h and ServiceRegistry.h
// ---------------------------------------------------------------------------
class IPlugin;
class ServiceRegistry;

// ---------------------------------------------------------------------------
// Plugin context passed to Eunoia_CreatePlugin
// ---------------------------------------------------------------------------
struct EunoiaPluginContext {
    ServiceRegistry* registry;           // The shared service registry
    const char*      pluginDataPath;     // Writable per-plugin data directory
};

// ---------------------------------------------------------------------------
// The three C exports every plugin DLL must provide
// ---------------------------------------------------------------------------
// Eunoia_GetPluginInfo  — return static metadata (called before create)
// Eunoia_CreatePlugin   — instantiate the plugin, return IPlugin*
// Eunoia_DestroyPlugin  — destroy the plugin instance
//
// These are function pointer typedefs for GetProcAddress / dlsym lookup:
typedef const EunoiaPluginInfo* (*PFN_Eunoia_GetPluginInfo)();
typedef IPlugin*                (*PFN_Eunoia_CreatePlugin)(const EunoiaPluginContext*);
typedef void                    (*PFN_Eunoia_DestroyPlugin)(IPlugin*);

// ---------------------------------------------------------------------------
// EUNOIA_DECLARE_PLUGIN — place at the bottom of your plugin's main .cpp
// ---------------------------------------------------------------------------
// Usage:
//   class MyPlugin : public IPlugin { ... };
//
//   static EunoiaPluginInfo s_info = { "my-plugin", "1.0.0", ... };
//   EUNOIA_DECLARE_PLUGIN(MyPlugin)
//
// This generates the three extern "C" exports the PluginManager looks for.
// ---------------------------------------------------------------------------
#define EUNOIA_DECLARE_PLUGIN(PluginClass, PluginInfoPtr)                        \
    EUNOIA_API const EunoiaPluginInfo* Eunoia_GetPluginInfo() {                  \
        return (PluginInfoPtr);                                                  \
    }                                                                            \
                                                                                \
    EUNOIA_API IPlugin* Eunoia_CreatePlugin(const EunoiaPluginContext* ctx) {    \
        return new PluginClass(ctx);                                             \
    }                                                                            \
                                                                                \
    EUNOIA_API void Eunoia_DestroyPlugin(IPlugin* plugin) {                     \
        delete plugin;                                                           \
    }

// ---------------------------------------------------------------------------
// Static Core Plugins (linked directly into the host binary)
// ---------------------------------------------------------------------------
#define EUNOIA_DECLARE_STATIC_PLUGIN(Name, PluginClass)                          \
    extern "C" const EunoiaPluginInfo* Eunoia_##Name##_GetPluginInfo() {         \
        return &s_##Name##Info;                                                  \
    }                                                                            \
    extern "C" IPlugin* Eunoia_##Name##_CreatePlugin(const EunoiaPluginContext* ctx) { \
        return new PluginClass(ctx);                                             \
    }                                                                            \
    extern "C" void Eunoia_##Name##_DestroyPlugin(IPlugin* plugin) {              \
        delete plugin;                                                           \
    }

#define EUNOIA_DECLARE_STATIC_PLUGIN_REF(Name)                                   \
    extern "C" const EunoiaPluginInfo* Eunoia_##Name##_GetPluginInfo();          \
    extern "C" IPlugin* Eunoia_##Name##_CreatePlugin(const EunoiaPluginContext*); \
    extern "C" void Eunoia_##Name##_DestroyPlugin(IPlugin* plugin);

#define EUNOIA_REGISTER_STATIC_PLUGIN(pluginManager, Name)                       \
    (pluginManager).RegisterStaticPlugin(*Eunoia_##Name##_GetPluginInfo(),       \
        [](const EunoiaPluginContext* ctx) -> IPlugin* {                         \
            return Eunoia_##Name##_CreatePlugin(ctx);                            \
        })
