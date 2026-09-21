#pragma once
// ============================================================================
// Eunoia Engine — Plugin Manager
// ============================================================================
// Discovers, loads, dependency-sorts, and manages the lifecycle of all plugins.
//
// Lifecycle:
//   1. LoadAll("plugins/")  — scan plugin.json files, topo-sort, load DLLs
//   2. InitAll()            — OnRegister() for all, then OnInit() for all
//   3. UpdateAll(dt)        — OnUpdate(dt) for all each frame
//   4. RenderAll()          — OnRender() for all each frame
//   5. ShutdownAll()        — OnShutdown() in reverse order, FreeLibrary
//
// Hot-reload:
//   ReloadPlugin("name")   — serialize → shutdown → free → rebuild →
//                             load → register → init → deserialize
// ============================================================================

#include "PluginAPI.h"
#include "IPlugin.h"
#include "ServiceRegistry.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <filesystem>

#ifdef _WIN32
    #include <windows.h>
    typedef HMODULE PluginHandle;
#else
    #include <dlfcn.h>
    typedef void* PluginHandle;
#endif

// ---------------------------------------------------------------------------
// Internal record for a loaded plugin
// ---------------------------------------------------------------------------
struct LoadedPlugin {
    std::string              name;
    std::string              version;
    EunoiaPluginCategory     category;
    bool                     isCore;
    std::vector<std::string> dependencies;
    std::vector<std::string> provides;

    PluginHandle             handle = nullptr;   // DLL handle (nullptr for core/static)
    IPlugin*                 instance = nullptr;
    EunoiaPluginInfo         info = {};

    // Function pointers from the DLL
    PFN_Eunoia_GetPluginInfo  pfnGetInfo    = nullptr;
    PFN_Eunoia_CreatePlugin   pfnCreate     = nullptr;
    PFN_Eunoia_DestroyPlugin  pfnDestroy    = nullptr;
};

// ---------------------------------------------------------------------------
// PluginManager
// ---------------------------------------------------------------------------
class PluginManager {
public:
    PluginManager();
    ~PluginManager();

    // -----------------------------------------------------------------------
    // Discovery & Loading
    // -----------------------------------------------------------------------

    // Scan the given directory recursively for plugin.json manifests,
    // topologically sort by dependencies, and load each plugin.
    bool LoadAll(const std::string& pluginsDir);

    // Register a statically-linked (core) plugin manually.
    // Call this before LoadAll() for core-tier plugins that aren't DLLs.
    void RegisterStaticPlugin(const EunoiaPluginInfo& info,
                              std::function<IPlugin*(const EunoiaPluginContext*)> factory);

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    // Phase 1: OnRegister() for all plugins (in dependency order)
    // Phase 2: OnInit() for all plugins (in dependency order)
    void InitAll();

    // Per-frame: OnUpdate(dt) for all plugins
    void UpdateAll(float deltaTime);

    // Per-frame: OnRender() for all plugins (after UpdateAll)
    void RenderAll();

    // Shutdown in reverse order: OnShutdown() → DestroyPlugin → FreeLibrary
    void ShutdownAll();

    // -----------------------------------------------------------------------
    // Hot Reload
    // -----------------------------------------------------------------------

    // Reload a single dynamic plugin by name.
    // Returns false if the plugin doesn't exist or doesn't support hot-reload.
    bool ReloadPlugin(const std::string& name);

    // -----------------------------------------------------------------------
    // Queries
    // -----------------------------------------------------------------------

    // Get the shared service registry
    ServiceRegistry& GetRegistry() { return m_registry; }
    const ServiceRegistry& GetRegistry() const { return m_registry; }

    // Get all loaded plugin names (in load order)
    std::vector<std::string> GetLoadedPluginNames() const;

    // Check if a plugin is loaded
    bool IsLoaded(const std::string& name) const;

    // Get plugin info by name
    const LoadedPlugin* GetPlugin(const std::string& name) const;

private:
    // Parse a single plugin.json manifest
    bool ParseManifest(const std::filesystem::path& jsonPath, LoadedPlugin& out);

    // Topological sort of plugins by dependencies
    bool TopologicalSort(std::vector<LoadedPlugin>& plugins);

    // Load a single plugin DLL and resolve its exports
    bool LoadPluginDLL(LoadedPlugin& plugin, const std::filesystem::path& pluginDir);

    // Unload a single plugin DLL
    void UnloadPluginDLL(LoadedPlugin& plugin);

    ServiceRegistry                          m_registry;
    std::vector<LoadedPlugin>                m_plugins;       // In dependency order
    std::unordered_map<std::string, size_t>  m_nameToIndex;   // name → index in m_plugins

    // Static plugin factories (registered before LoadAll)
    struct StaticPluginEntry {
        EunoiaPluginInfo info;
        std::function<IPlugin*(const EunoiaPluginContext*)> factory;
    };
    std::vector<StaticPluginEntry> m_staticPlugins;
};
