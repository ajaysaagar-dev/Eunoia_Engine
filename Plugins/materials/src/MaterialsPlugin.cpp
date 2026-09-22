// ============================================================================
// Eunoia Engine — Materials Plugin Implementation
// ============================================================================

#include "Materials/IMaterialSystem.h"
#include "PluginAPI.h"
#include "IPlugin.h"
#include "ServiceRegistry.h"

#include <iostream>

class MaterialSystemImpl : public IMaterialSystem {
public:
    bool LoadMaterial(const std::string& /*path*/) override { return true; }
    bool SaveMaterial(const std::string& /*path*/) override { return true; }
};

class MaterialsPlugin : public IPlugin {
public:
    explicit MaterialsPlugin(const EunoiaPluginContext* /*ctx*/) {}
    ~MaterialsPlugin() override = default;

    const char* GetName() const override { return "materials"; }
    bool SupportsHotReload() const override { return true; }

    void OnRegister(ServiceRegistry& registry) override {
        std::cout << "[MaterialsPlugin] Registering IMaterialSystem service..." << std::endl;
        registry.Register<IMaterialSystem>("IMaterialSystem", &m_matSystem);
    }

    void OnInit() override {
        std::cout << "[MaterialsPlugin] Materials plugin initialized." << std::endl;
    }

    void OnShutdown() override {
        std::cout << "[MaterialsPlugin] Materials plugin shutdown." << std::endl;
    }

private:
    MaterialSystemImpl m_matSystem;
};

static EunoiaPluginInfo s_materialsInfo = {
    "materials",
    "1.0.0",
    "Material system and shader parameter bindings",
    "Eunoia Team",
    EUNOIA_PLUGIN_API_VERSION,
    EunoiaPluginCategory::Materials,
    false // dynamic tier
};

EUNOIA_DECLARE_PLUGIN(MaterialsPlugin, &s_materialsInfo)
