// ============================================================================
// Eunoia Engine — Plugin Manager Implementation
// ============================================================================

#include <EunoiaPluginCore/PluginManager.h>

#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>
#include <algorithm>
#include <queue>
#include <set>
#include <stdexcept>

using json = nlohmann::json;

// ============================================================================
// Constructor / Destructor
// ============================================================================

PluginManager::PluginManager() = default;

PluginManager::~PluginManager() {
    ShutdownAll();
}

// ============================================================================
// Manifest Parsing
// ============================================================================

static EunoiaPluginCategory ParseCategory(const std::string& cat) {
    if (cat == "engine")      return EunoiaPluginCategory::Engine;
    if (cat == "platform")    return EunoiaPluginCategory::Platform;
    if (cat == "rhi")         return EunoiaPluginCategory::RHI;
    if (cat == "rendering")   return EunoiaPluginCategory::Rendering;
    if (cat == "scene")       return EunoiaPluginCategory::Scene;
    if (cat == "levels")      return EunoiaPluginCategory::Levels;
    if (cat == "assets")      return EunoiaPluginCategory::Assets;
    if (cat == "primitives")  return EunoiaPluginCategory::Primitives;
    if (cat == "materials")   return EunoiaPluginCategory::Materials;
    if (cat == "lights")      return EunoiaPluginCategory::Lights;
    if (cat == "behaviours")  return EunoiaPluginCategory::Behaviours;
    if (cat == "editor")      return EunoiaPluginCategory::Editor;
    if (cat == "cameras" || cat == "Cameras") return EunoiaPluginCategory::Cameras;
    return EunoiaPluginCategory::Custom;
}

bool PluginManager::ParseManifest(const std::filesystem::path& jsonPath, LoadedPlugin& out) {
    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        std::cerr << "[PluginManager] Failed to open manifest: " << jsonPath << std::endl;
        return false;
    }

    json j;
    try {
        file >> j;
    } catch (const json::parse_error& e) {
        std::cerr << "[PluginManager] JSON parse error in " << jsonPath << ": " << e.what() << std::endl;
        return false;
    }

    // Required fields
    if (!j.contains("name") || !j.contains("version") || !j.contains("api_version")) {
        std::cerr << "[PluginManager] Manifest missing required fields: " << jsonPath << std::endl;
        return false;
    }

    out.name     = j["name"].get<std::string>();
    out.version  = j["version"].get<std::string>();
    out.isCore   = j.value("core", false);
    out.category = ParseCategory(j.value("category", "custom"));

    uint32_t apiVer = j["api_version"].get<uint32_t>();
    if (apiVer != EUNOIA_PLUGIN_API_VERSION) {
        std::cerr << "[PluginManager] API version mismatch for plugin '" << out.name
                  << "': expected " << EUNOIA_PLUGIN_API_VERSION
                  << ", got " << apiVer << std::endl;
        return false;
    }

    // Optional arrays
    if (j.contains("dependencies") && j["dependencies"].is_array()) {
        for (const auto& dep : j["dependencies"]) {
            out.dependencies.push_back(dep.get<std::string>());
        }
    }
    if (j.contains("provides") && j["provides"].is_array()) {
        for (const auto& prov : j["provides"]) {
            out.provides.push_back(prov.get<std::string>());
        }
    }

    // Fill info struct
    out.info.name       = out.name.c_str();
    out.info.version    = out.version.c_str();
    out.info.apiVersion = apiVer;
    out.info.category   = out.category;
    out.info.isCore     = out.isCore;
    out.info.description = j.value("description", "").c_str();
    out.info.author      = j.value("author", "").c_str();

    return true;
}

// ============================================================================
// Topological Sort (Kahn's algorithm)
// ============================================================================

bool PluginManager::TopologicalSort(std::vector<LoadedPlugin>& plugins) {
    std::unordered_map<std::string, size_t> nameToIdx;
    for (size_t i = 0; i < plugins.size(); ++i) {
        nameToIdx[plugins[i].name] = i;
    }

    // Build adjacency and in-degree
    std::vector<std::vector<size_t>> adj(plugins.size());
    std::vector<int> inDeg(plugins.size(), 0);

    for (size_t i = 0; i < plugins.size(); ++i) {
        for (const auto& dep : plugins[i].dependencies) {
            auto it = nameToIdx.find(dep);
            if (it == nameToIdx.end()) {
                // Check if it's a registered static plugin
                bool foundStatic = false;
                for (const auto& sp : m_staticPlugins) {
                    if (sp.info.name && std::string(sp.info.name) == dep) {
                        foundStatic = true;
                        break;
                    }
                }
                if (!foundStatic) {
                    std::cerr << "[PluginManager] FATAL: Plugin '" << plugins[i].name
                              << "' depends on '" << dep << "' which is not found." << std::endl;
                    return false;
                }
                continue;  // Static plugin dep, already loaded — skip edge
            }
            adj[it->second].push_back(i);  // dep → i  (dep must come before i)
            inDeg[i]++;
        }
    }

    // Kahn's BFS
    std::queue<size_t> q;
    for (size_t i = 0; i < plugins.size(); ++i) {
        if (inDeg[i] == 0) q.push(i);
    }

    std::vector<LoadedPlugin> sorted;
    sorted.reserve(plugins.size());

    while (!q.empty()) {
        size_t cur = q.front();
        q.pop();
        sorted.push_back(std::move(plugins[cur]));
        for (size_t next : adj[cur]) {
            if (--inDeg[next] == 0) {
                q.push(next);
            }
        }
    }

    if (sorted.size() != plugins.size()) {
        std::cerr << "[PluginManager] FATAL: Circular dependency detected among plugins." << std::endl;
        return false;
    }

    // Core plugins first, then dynamic
    std::stable_partition(sorted.begin(), sorted.end(),
                          [](const LoadedPlugin& p) { return p.isCore; });

    plugins = std::move(sorted);
    return true;
}

// ============================================================================
// DLL Loading
// ============================================================================

bool PluginManager::LoadPluginDLL(LoadedPlugin& plugin, const std::filesystem::path& pluginDir) {
#ifdef _WIN32
    // Look for <name>.dll in the plugin directory or in the build output
    std::filesystem::path dllPath = pluginDir / (plugin.name + ".dll");
    if (!std::filesystem::exists(dllPath)) {
        // Try Build/Engine/ directory
        dllPath = std::filesystem::path("Build") / "Engine" / (plugin.name + ".dll");
    }
    if (!std::filesystem::exists(dllPath)) {
        // Try current working directory
        dllPath = std::filesystem::path(plugin.name + ".dll");
    }
    if (!std::filesystem::exists(dllPath)) {
        std::cerr << "[PluginManager] DLL not found for plugin '" << plugin.name
                  << "' at " << dllPath << std::endl;
        return false;
    }

    plugin.handle = LoadLibraryA(dllPath.string().c_str());
    if (!plugin.handle) {
        DWORD err = GetLastError();
        std::cerr << "[PluginManager] LoadLibrary failed for '" << plugin.name
                  << "': error " << err << std::endl;
        return false;
    }

    plugin.pfnGetInfo = (PFN_Eunoia_GetPluginInfo)GetProcAddress(plugin.handle, "Eunoia_GetPluginInfo");
    plugin.pfnCreate  = (PFN_Eunoia_CreatePlugin)GetProcAddress(plugin.handle, "Eunoia_CreatePlugin");
    plugin.pfnDestroy = (PFN_Eunoia_DestroyPlugin)GetProcAddress(plugin.handle, "Eunoia_DestroyPlugin");

    if (!plugin.pfnGetInfo || !plugin.pfnCreate || !plugin.pfnDestroy) {
        std::cerr << "[PluginManager] Missing exports in DLL for plugin '" << plugin.name << "'" << std::endl;
        FreeLibrary(plugin.handle);
        plugin.handle = nullptr;
        return false;
    }
#else
    std::filesystem::path soPath = pluginDir / ("lib" + plugin.name + ".so");
    plugin.handle = dlopen(soPath.c_str(), RTLD_NOW);
    if (!plugin.handle) {
        std::cerr << "[PluginManager] dlopen failed for '" << plugin.name
                  << "': " << dlerror() << std::endl;
        return false;
    }
    plugin.pfnGetInfo = (PFN_Eunoia_GetPluginInfo)dlsym(plugin.handle, "Eunoia_GetPluginInfo");
    plugin.pfnCreate  = (PFN_Eunoia_CreatePlugin)dlsym(plugin.handle, "Eunoia_CreatePlugin");
    plugin.pfnDestroy = (PFN_Eunoia_DestroyPlugin)dlsym(plugin.handle, "Eunoia_DestroyPlugin");

    if (!plugin.pfnGetInfo || !plugin.pfnCreate || !plugin.pfnDestroy) {
        std::cerr << "[PluginManager] Missing exports in SO for plugin '" << plugin.name << "'" << std::endl;
        dlclose(plugin.handle);
        plugin.handle = nullptr;
        return false;
    }
#endif

    // Validate API version from DLL
    const EunoiaPluginInfo* dllInfo = plugin.pfnGetInfo();
    if (!dllInfo || dllInfo->apiVersion != EUNOIA_PLUGIN_API_VERSION) {
        std::cerr << "[PluginManager] API version mismatch in DLL for '" << plugin.name << "'" << std::endl;
        UnloadPluginDLL(plugin);
        return false;
    }

    return true;
}

void PluginManager::UnloadPluginDLL(LoadedPlugin& plugin) {
    if (plugin.handle) {
#ifdef _WIN32
        FreeLibrary(plugin.handle);
#else
        dlclose(plugin.handle);
#endif
        plugin.handle = nullptr;
    }
    plugin.pfnGetInfo = nullptr;
    plugin.pfnCreate  = nullptr;
    plugin.pfnDestroy = nullptr;
}

// ============================================================================
// Static Plugin Registration
// ============================================================================

void PluginManager::RegisterStaticPlugin(const EunoiaPluginInfo& info,
                                          std::function<IPlugin*(const EunoiaPluginContext*)> factory) {
    m_staticPlugins.push_back({info, std::move(factory)});
}

// ============================================================================
// LoadAll — Discovery + Loading
// ============================================================================

bool PluginManager::LoadAll(const std::string& pluginsDir) {
    std::vector<LoadedPlugin> discovered;

    // 1. Scan for plugin.json manifests
    if (std::filesystem::exists(pluginsDir)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(pluginsDir)) {
            if (entry.is_regular_file() && entry.path().filename() == "plugin.json") {
                LoadedPlugin plugin;
                if (ParseManifest(entry.path(), plugin)) {
                    // Store the directory containing plugin.json for DLL lookup
                    std::cout << "[PluginManager] Discovered plugin: " << plugin.name
                              << " v" << plugin.version
                              << (plugin.isCore ? " [core]" : " [dynamic]")
                              << std::endl;
                    discovered.push_back(std::move(plugin));
                }
            }
        }
    }

    // 2. Add static plugins that weren't discovered as manifests
    for (const auto& sp : m_staticPlugins) {
        bool alreadyFound = false;
        for (const auto& d : discovered) {
            if (d.name == sp.info.name) { alreadyFound = true; break; }
        }
        if (!alreadyFound) {
            LoadedPlugin lp;
            lp.name     = sp.info.name ? sp.info.name : "";
            lp.version  = sp.info.version ? sp.info.version : "1.0.0";
            lp.isCore   = sp.info.isCore;
            lp.category = sp.info.category;
            lp.info     = sp.info;
            discovered.push_back(std::move(lp));
        }
    }

    if (discovered.empty()) {
        std::cout << "[PluginManager] No plugins discovered." << std::endl;
        return true;
    }

    // 3. Topological sort
    if (!TopologicalSort(discovered)) {
        return false;
    }

    // 4. Load each plugin
    for (auto& plugin : discovered) {
        if (plugin.isCore) {
            // Core plugins are statically linked — find their factory
            for (const auto& sp : m_staticPlugins) {
                if (sp.info.name && std::string(sp.info.name) == plugin.name) {
                    EunoiaPluginContext ctx;
                    ctx.registry = &m_registry;
                    ctx.pluginDataPath = nullptr;
                    plugin.instance = sp.factory(&ctx);
                    break;
                }
            }
            if (!plugin.instance) {
                std::cerr << "[PluginManager] Core plugin '" << plugin.name
                          << "' has no registered factory." << std::endl;
                // Non-fatal for now during incremental migration
            }
        } else {
            // Dynamic plugins — find the plugin.json directory for DLL lookup
            std::filesystem::path searchDir = pluginsDir;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(pluginsDir)) {
                if (entry.is_regular_file() && entry.path().filename() == "plugin.json") {
                    // Quick check: does this manifest match?
                    std::ifstream f(entry.path());
                    json j;
                    try { f >> j; } catch (...) { continue; }
                    if (j.value("name", "") == plugin.name) {
                        searchDir = entry.path().parent_path();
                        break;
                    }
                }
            }

            if (LoadPluginDLL(plugin, searchDir)) {
                EunoiaPluginContext ctx;
                ctx.registry = &m_registry;
                ctx.pluginDataPath = nullptr;
                plugin.instance = plugin.pfnCreate(&ctx);
            } else {
                std::cerr << "[PluginManager] Failed to load DLL for plugin '" << plugin.name << "'" << std::endl;
                // Non-fatal during incremental migration
            }
        }
    }

    // Store and index
    m_plugins = std::move(discovered);
    m_nameToIndex.clear();
    for (size_t i = 0; i < m_plugins.size(); ++i) {
        m_nameToIndex[m_plugins[i].name] = i;
    }

    std::cout << "[PluginManager] Loaded " << m_plugins.size() << " plugins." << std::endl;
    return true;
}

// ============================================================================
// InitAll — OnRegister then OnInit
// ============================================================================

void PluginManager::InitAll() {
    // Phase 1: OnRegister for all (publish services)
    for (auto& plugin : m_plugins) {
        if (plugin.instance) {
            std::cout << "[PluginManager] OnRegister: " << plugin.name << std::endl;
            plugin.instance->OnRegister(m_registry);
        }
    }

    // Phase 2: OnInit for all (consume services)
    for (auto& plugin : m_plugins) {
        if (plugin.instance) {
            std::cout << "[PluginManager] OnInit: " << plugin.name << std::endl;
            plugin.instance->OnInit();
        }
    }
}

// ============================================================================
// UpdateAll / RenderAll
// ============================================================================

void PluginManager::UpdateAll(float deltaTime) {
    for (auto& plugin : m_plugins) {
        if (plugin.instance) {
            plugin.instance->OnUpdate(deltaTime);
        }
    }
}

void PluginManager::RenderAll() {
    for (auto& plugin : m_plugins) {
        if (plugin.instance) {
            plugin.instance->OnRender();
        }
    }
}

// ============================================================================
// ShutdownAll
// ============================================================================

void PluginManager::ShutdownAll() {
    // Reverse order shutdown
    for (int i = (int)m_plugins.size() - 1; i >= 0; --i) {
        auto& plugin = m_plugins[i];
        if (plugin.instance) {
            std::cout << "[PluginManager] OnShutdown: " << plugin.name << std::endl;
            plugin.instance->OnShutdown();

            if (plugin.pfnDestroy) {
                plugin.pfnDestroy(plugin.instance);
            } else {
                delete plugin.instance;
            }
            plugin.instance = nullptr;
        }
        UnloadPluginDLL(plugin);
    }

    m_plugins.clear();
    m_nameToIndex.clear();
    m_registry.Clear();
}

// ============================================================================
// Hot Reload
// ============================================================================

bool PluginManager::ReloadPlugin(const std::string& name) {
    auto it = m_nameToIndex.find(name);
    if (it == m_nameToIndex.end()) {
        std::cerr << "[PluginManager] Cannot reload: plugin '" << name << "' not found." << std::endl;
        return false;
    }

    auto& plugin = m_plugins[it->second];

    if (plugin.isCore) {
        std::cerr << "[PluginManager] Cannot reload core plugin '" << name << "'." << std::endl;
        return false;
    }

    if (!plugin.instance || !plugin.instance->SupportsHotReload()) {
        std::cerr << "[PluginManager] Plugin '" << name << "' does not support hot reload." << std::endl;
        return false;
    }

    std::cout << "[PluginManager] Hot-reloading plugin: " << name << std::endl;

    // 1. Serialize state
    std::string savedState = plugin.instance->OnSerializeState();

    // 2. Shutdown
    plugin.instance->OnShutdown();
    if (plugin.pfnDestroy) {
        plugin.pfnDestroy(plugin.instance);
    }
    plugin.instance = nullptr;

    // 3. Unload DLL
    UnloadPluginDLL(plugin);

    // 4. Reload DLL (assumes it was rebuilt externally)
    // Find the plugin directory by scanning again
    std::filesystem::path pluginDir = "plugins";
    for (const auto& entry : std::filesystem::recursive_directory_iterator("plugins")) {
        if (entry.is_regular_file() && entry.path().filename() == "plugin.json") {
            std::ifstream f(entry.path());
            json j;
            try { f >> j; } catch (...) { continue; }
            if (j.value("name", "") == name) {
                pluginDir = entry.path().parent_path();
                break;
            }
        }
    }

    if (!LoadPluginDLL(plugin, pluginDir)) {
        std::cerr << "[PluginManager] Failed to reload DLL for '" << name << "'." << std::endl;
        return false;
    }

    // 5. Create + Register + Init
    EunoiaPluginContext ctx;
    ctx.registry = &m_registry;
    ctx.pluginDataPath = nullptr;
    plugin.instance = plugin.pfnCreate(&ctx);

    if (plugin.instance) {
        plugin.instance->OnRegister(m_registry);
        plugin.instance->OnInit();
        plugin.instance->OnDeserializeState(savedState);
        std::cout << "[PluginManager] Successfully hot-reloaded: " << name << std::endl;
        return true;
    }

    return false;
}

// ============================================================================
// Queries
// ============================================================================

std::vector<std::string> PluginManager::GetLoadedPluginNames() const {
    std::vector<std::string> names;
    names.reserve(m_plugins.size());
    for (const auto& p : m_plugins) {
        names.push_back(p.name);
    }
    return names;
}

bool PluginManager::IsLoaded(const std::string& name) const {
    return m_nameToIndex.find(name) != m_nameToIndex.end();
}

const LoadedPlugin* PluginManager::GetPlugin(const std::string& name) const {
    auto it = m_nameToIndex.find(name);
    if (it == m_nameToIndex.end()) return nullptr;
    return &m_plugins[it->second];
}
