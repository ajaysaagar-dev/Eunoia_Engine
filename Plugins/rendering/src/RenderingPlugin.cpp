// ============================================================================
// Eunoia Engine — Rendering Plugin Implementation
// ============================================================================

#include "Rendering/IRenderer.h"
#include "PluginAPI.h"
#include "IPlugin.h"
#include "ServiceRegistry.h"

#include <iostream>

class SceneRendererService : public IRenderer {
public:
    void RenderScene(Scene* /*scene*/, const OrbitCamera* /*camera*/) override {
        // Scene rendering pass delegation
    }
};

class RenderingPlugin : public IPlugin {
public:
    explicit RenderingPlugin(const EunoiaPluginContext* /*ctx*/) {}
    ~RenderingPlugin() override = default;

    const char* GetName() const override { return "rendering"; }
    bool SupportsHotReload() const override { return true; }

    void OnRegister(ServiceRegistry& registry) override {
        std::cout << "[RenderingPlugin] Registering IRenderer service..." << std::endl;
        registry.Register<IRenderer>("IRenderer", &m_rendererService);
    }

    void OnInit() override {
        std::cout << "[RenderingPlugin] Rendering plugin initialized." << std::endl;
    }

    void OnShutdown() override {
        std::cout << "[RenderingPlugin] Rendering plugin shutdown." << std::endl;
    }

private:
    SceneRendererService m_rendererService;
};

static EunoiaPluginInfo s_renderingInfo = {
    "rendering",
    "1.0.0",
    "Scene Renderer and Shading pipeline",
    "Eunoia Team",
    EUNOIA_PLUGIN_API_VERSION,
    EunoiaPluginCategory::Rendering,
    false // dynamic tier
};

EUNOIA_DECLARE_PLUGIN(RenderingPlugin, &s_renderingInfo)
