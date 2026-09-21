#pragma once
// ============================================================================
// Eunoia Engine — Service Registry
// ============================================================================
// String-keyed, typed service registry that replaces direct cross-module
// #include calls.  All inter-plugin communication goes through here.
//
// Why string keys instead of typeid?
//   RTTI identity isn't guaranteed stable across separately-compiled DLLs
//   (even with the same compiler version).  String keys are explicit,
//   debuggable, and cross-DLL safe.
//
// Usage:
//   // In OnRegister():
//   registry.Register<IRenderer>("IRenderer", myRendererPtr);
//
//   // In OnInit() of a consuming plugin:
//   auto* renderer = registry.Resolve<IRenderer>("IRenderer");   // nullptr if missing
//   auto* renderer = registry.Require<IRenderer>("IRenderer");   // asserts if missing
// ============================================================================

#include <string>
#include <unordered_map>
#include <cassert>
#include <iostream>
#include <vector>
#include <mutex>

class ServiceRegistry {
public:
    // -----------------------------------------------------------------------
    // Register a service by name.  Overwrites any previous registration
    // with the same name (useful for hot-reload).
    // -----------------------------------------------------------------------
    template <typename T>
    void Register(const std::string& name, T* service) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_services[name] = static_cast<void*>(service);
    }

    // -----------------------------------------------------------------------
    // Resolve a service by name.  Returns nullptr if not registered.
    // -----------------------------------------------------------------------
    template <typename T>
    T* Resolve(const std::string& name) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_services.find(name);
        if (it == m_services.end()) return nullptr;
        return static_cast<T*>(it->second);
    }

    // -----------------------------------------------------------------------
    // Require a service by name.  Asserts and logs if not found.
    // Use this when a missing dependency is a fatal configuration error.
    // -----------------------------------------------------------------------
    template <typename T>
    T* Require(const std::string& name) const {
        T* service = Resolve<T>(name);
        if (!service) {
            std::cerr << "[ServiceRegistry] FATAL: Required service '"
                      << name << "' not found. Check plugin dependencies." << std::endl;
            assert(false && "Required service not found in ServiceRegistry");
        }
        return service;
    }

    // -----------------------------------------------------------------------
    // Unregister a service by name (used during hot-reload teardown).
    // -----------------------------------------------------------------------
    void Unregister(const std::string& name) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_services.erase(name);
    }

    // -----------------------------------------------------------------------
    // Check if a service is registered.
    // -----------------------------------------------------------------------
    bool Has(const std::string& name) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_services.find(name) != m_services.end();
    }

    // -----------------------------------------------------------------------
    // Get all registered service names (for debugging / introspection).
    // -----------------------------------------------------------------------
    std::vector<std::string> GetRegisteredNames() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<std::string> names;
        names.reserve(m_services.size());
        for (const auto& [name, _] : m_services) {
            names.push_back(name);
        }
        return names;
    }

    // -----------------------------------------------------------------------
    // Clear all registrations (used during full shutdown).
    // -----------------------------------------------------------------------
    void Clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_services.clear();
    }

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, void*> m_services;
};
