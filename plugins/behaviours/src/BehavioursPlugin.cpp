// ============================================================================
// Eunoia Engine — Behaviours Plugin Implementation
// ============================================================================

#include "Behaviours/IBehaviourService.h"
#include "PluginAPI.h"
#include "IPlugin.h"
#include "ServiceRegistry.h"

#include <iostream>

class BehaviourServiceImpl : public IBehaviourService {
public:
    size_t GetRegisteredCount() const override { return 0; }
};

class BehavioursPlugin : public IPlugin {
public:
    explicit BehavioursPlugin(const EunoiaPluginContext* /*ctx*/) {}
    ~BehavioursPlugin() override = default;

    const char* GetName() const override { return "behaviours"; }
    bool SupportsHotReload() const override { return true; }

    void OnRegister(ServiceRegistry& registry) override {
        std::cout << "[BehavioursPlugin] Registering IBehaviourService service..." << std::endl;
        registry.Register<IBehaviourService>("IBehaviourService", &m_service);
    }

    void OnInit() override {
        std::cout << "[BehavioursPlugin] Behaviours plugin initialized." << std::endl;
    }

    void OnShutdown() override {
        std::cout << "[BehavioursPlugin] Behaviours plugin shutdown." << std::endl;
    }

private:
    BehaviourServiceImpl m_service;
};

static EunoiaPluginInfo s_behavioursInfo = {
    "behaviours",
    "1.0.0",
    "Component gameplay scripting and reflection",
    "Eunoia Team",
    EUNOIA_PLUGIN_API_VERSION,
    EunoiaPluginCategory::Behaviours,
    false // dynamic tier
};

EUNOIA_DECLARE_PLUGIN(BehavioursPlugin, &s_behavioursInfo)
