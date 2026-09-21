// ============================================================================
// Eunoia Engine — Lights Plugin Implementation
// ============================================================================

#include "Lights/ILightSystem.h"
#include "PluginAPI.h"
#include "IPlugin.h"
#include "ServiceRegistry.h"

#include <iostream>

class LightSystemImpl : public ILightSystem {
public:
    uint32_t GetActiveLightCount() const override { return 0; }
};

class LightsPlugin : public IPlugin {
public:
    explicit LightsPlugin(const EunoiaPluginContext* /*ctx*/) {}
    ~LightsPlugin() override = default;

    const char* GetName() const override { return "lights"; }
    bool SupportsHotReload() const override { return true; }

    void OnRegister(ServiceRegistry& registry) override {
        std::cout << "[LightsPlugin] Registering ILightSystem service..." << std::endl;
        registry.Register<ILightSystem>("ILightSystem", &m_lightSystem);
    }

    void OnInit() override {
        std::cout << "[LightsPlugin] Lights plugin initialized." << std::endl;
    }

    void OnShutdown() override {
        std::cout << "[LightsPlugin] Lights plugin shutdown." << std::endl;
    }

private:
    LightSystemImpl m_lightSystem;
};

static EunoiaPluginInfo s_lightsInfo = {
    "lights",
    "1.0.0",
    "Lighting system and shadow properties",
    "Eunoia Team",
    EUNOIA_PLUGIN_API_VERSION,
    EunoiaPluginCategory::Lights,
    false // dynamic tier
};

EUNOIA_DECLARE_PLUGIN(LightsPlugin, &s_lightsInfo)
