#pragma once
// ============================================================================
// Eunoia Engine — IPlugin Interface
// ============================================================================
// Every plugin implements this C++ interface.  The lifecycle is:
//
//   1. Eunoia_CreatePlugin()     — construct the plugin
//   2. OnRegister(registry)      — publish your services into the registry
//   3. OnInit()                  — resolve/consume other plugins' services
//   4. OnUpdate(dt)              — called each frame (opt-in)
//   5. OnShutdown()              — cleanup before destruction
//   6. Eunoia_DestroyPlugin()    — free the plugin
//
// Hot-reload (dynamic plugins only):
//   OnSerializeState()   → OnShutdown() → FreeLibrary → rebuild →
//   LoadLibrary → OnRegister() → OnInit() → OnDeserializeState()
// ============================================================================

#include <string>

class ServiceRegistry;

class IPlugin {
public:
    virtual ~IPlugin() = default;

    // -----------------------------------------------------------------------
    // Phase 1: Registration — publish services into the shared registry.
    // Called in topological dependency order.  At this point, only plugins
    // earlier in the order have registered — do NOT try to Resolve() here.
    // -----------------------------------------------------------------------
    virtual void OnRegister(ServiceRegistry& registry) = 0;

    // -----------------------------------------------------------------------
    // Phase 2: Initialization — all plugins have registered.
    // Safe to Resolve<T>() / Require<T>() dependencies from the registry.
    // -----------------------------------------------------------------------
    virtual void OnInit() = 0;

    // -----------------------------------------------------------------------
    // Per-frame update — called by PluginManager::UpdateAll(dt).
    // Default is a no-op; override only if your plugin needs per-frame work.
    // -----------------------------------------------------------------------
    virtual void OnUpdate(float /*deltaTime*/) {}

    // -----------------------------------------------------------------------
    // Render — called after all OnUpdate() calls, for plugins that draw.
    // Default is a no-op.
    // -----------------------------------------------------------------------
    virtual void OnRender() {}

    // -----------------------------------------------------------------------
    // Shutdown — cleanup resources, unregister services.
    // Called in reverse dependency order.
    // -----------------------------------------------------------------------
    virtual void OnShutdown() = 0;

    // -----------------------------------------------------------------------
    // Hot-reload support (dynamic plugins only)
    // -----------------------------------------------------------------------
    
    // Return true if this plugin can be unloaded and reloaded at runtime.
    // Core-tier plugins must return false.
    virtual bool SupportsHotReload() const { return false; }

    // Serialize plugin state to a string (JSON, binary, etc.) before unload.
    // Return empty string if stateless.
    virtual std::string OnSerializeState() { return ""; }

    // Restore plugin state from a previously serialized string after reload.
    virtual void OnDeserializeState(const std::string& /*state*/) {}

    // -----------------------------------------------------------------------
    // Plugin name — convenience accessor (should match PluginInfo.name)
    // -----------------------------------------------------------------------
    virtual const char* GetName() const = 0;
};
