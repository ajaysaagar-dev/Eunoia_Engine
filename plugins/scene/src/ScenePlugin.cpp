// ============================================================================
// Eunoia Engine — Scene Plugin Implementation
// ============================================================================

#include "Scene/ISceneManager.h"
#include "PluginAPI.h"
#include "IPlugin.h"
#include "ServiceRegistry.h"

#include <iostream>

class SceneManagerService : public ISceneManager {
public:
    Scene* GetActiveScene() override { return m_activeScene; }
    void SetActiveScene(Scene* scene) override { m_activeScene = scene; }
private:
    Scene* m_activeScene = nullptr;
};

class ScenePlugin : public IPlugin {
public:
    explicit ScenePlugin(const EunoiaPluginContext* /*ctx*/) {}
    ~ScenePlugin() override = default;

    const char* GetName() const override { return "scene"; }
    bool SupportsHotReload() const override { return true; }

    void OnRegister(ServiceRegistry& registry) override {
        std::cout << "[ScenePlugin] Registering ISceneManager service..." << std::endl;
        registry.Register<ISceneManager>("ISceneManager", &m_sceneService);
    }

    void OnInit() override {
        std::cout << "[ScenePlugin] Scene plugin initialized." << std::endl;
    }

    void OnShutdown() override {
        std::cout << "[ScenePlugin] Scene plugin shutdown." << std::endl;
    }

private:
    SceneManagerService m_sceneService;
};

static EunoiaPluginInfo s_sceneInfo = {
    "scene",
    "1.0.0",
    "Scene Graph and Camera Management",
    "Eunoia Team",
    EUNOIA_PLUGIN_API_VERSION,
    EunoiaPluginCategory::Scene,
    false // dynamic tier
};

EUNOIA_DECLARE_PLUGIN(ScenePlugin, &s_sceneInfo)
