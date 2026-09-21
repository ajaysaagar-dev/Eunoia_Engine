// ============================================================================
// Eunoia Engine — Core Engine Plugin Implementation
// ============================================================================

#include "Engine/ILog.h"
#include "EngineCore/Log.h"
#include "EngineCore/JobSystem.h"
#include "PluginAPI.h"
#include "IPlugin.h"
#include "ServiceRegistry.h"

#include <iostream>

class EngineLogService : public ILog {
public:
    void Info(const std::string& cat, const std::string& msg) override {
        EngineCore::Log::Get().Info(cat, msg);
    }
    void Warning(const std::string& cat, const std::string& msg) override {
        EngineCore::Log::Get().Warning(cat, msg);
    }
    void Success(const std::string& cat, const std::string& msg) override {
        EngineCore::Log::Get().Success(cat, msg);
    }
    void Error(const std::string& cat, const std::string& msg) override {
        EngineCore::Log::Get().Error(cat, msg);
    }
};

class EnginePlugin : public IPlugin {
public:
    explicit EnginePlugin(const EunoiaPluginContext* /*ctx*/) {}
    ~EnginePlugin() override = default;

    const char* GetName() const override { return "engine"; }
    bool SupportsHotReload() const override { return false; } // Core plugin

    void OnRegister(ServiceRegistry& registry) override {
        std::cout << "[EnginePlugin] Registering ILog & IJobSystem services..." << std::endl;
        registry.Register<ILog>("ILog", &m_logService);
    }

    void OnInit() override {
        std::cout << "[EnginePlugin] Core Engine plugin initialized." << std::endl;
    }

    void OnShutdown() override {
        std::cout << "[EnginePlugin] Core Engine plugin shutdown." << std::endl;
    }

private:
    EngineLogService m_logService;
};

// Static export / factory registration
static EunoiaPluginInfo s_EngineInfo = {
    "engine",
    "1.0.0",
    "Engine Core services",
    "Eunoia Team",
    EUNOIA_PLUGIN_API_VERSION,
    EunoiaPluginCategory::Engine,
    true // isCore
};

EUNOIA_DECLARE_STATIC_PLUGIN(Engine, EnginePlugin)
