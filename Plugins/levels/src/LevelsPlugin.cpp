// ============================================================================
// Eunoia Engine — Levels Plugin Implementation
// ============================================================================

#include "Levels/ILevelSerializer.h"
#include "PluginAPI.h"
#include "IPlugin.h"
#include "ServiceRegistry.h"

#include <iostream>

class LevelSerializerImpl : public ILevelSerializer {
public:
    bool SaveLevel(const std::string& /*path*/, const Scene& /*scene*/) override { return true; }
    bool LoadLevel(const std::string& /*path*/, Scene& /*scene*/) override { return true; }
};

class LevelsPlugin : public IPlugin {
public:
    explicit LevelsPlugin(const EunoiaPluginContext* /*ctx*/) {}
    ~LevelsPlugin() override = default;

    const char* GetName() const override { return "levels"; }
    bool SupportsHotReload() const override { return true; }

    void OnRegister(ServiceRegistry& registry) override {
        std::cout << "[LevelsPlugin] Registering ILevelSerializer service..." << std::endl;
        registry.Register<ILevelSerializer>("ILevelSerializer", &m_serializer);
    }

    void OnInit() override {
        std::cout << "[LevelsPlugin] Levels plugin initialized." << std::endl;
    }

    void OnShutdown() override {
        std::cout << "[LevelsPlugin] Levels plugin shutdown." << std::endl;
    }

private:
    LevelSerializerImpl m_serializer;
};

static EunoiaPluginInfo s_levelsInfo = {
    "levels",
    "1.0.0",
    "Level serialization and streaming",
    "Eunoia Team",
    EUNOIA_PLUGIN_API_VERSION,
    EunoiaPluginCategory::Levels,
    false // dynamic tier
};

EUNOIA_DECLARE_PLUGIN(LevelsPlugin, &s_levelsInfo)
