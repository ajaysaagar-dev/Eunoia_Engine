// ============================================================================
// Eunoia Engine — Viewport Editor Panel Plugin
// ============================================================================

#include "IEditorPanel.h"
#include "PluginAPI.h"
#include "IPlugin.h"
#include "ServiceRegistry.h"

#include <iostream>

class ViewportPanelImpl : public IEditorPanel {
public:
    const char* GetPanelName() const override { return "Viewport"; }
    void Draw() override {}
    bool IsOpen() const override { return m_open; }
    void SetOpen(bool open) override { m_open = open; }
private:
    bool m_open = true;
};

class ViewportPlugin : public IPlugin {
public:
    explicit ViewportPlugin(const EunoiaPluginContext* /*ctx*/) {}
    ~ViewportPlugin() override = default;

    const char* GetName() const override { return "editor-viewport"; }
    bool SupportsHotReload() const override { return true; }

    void OnRegister(ServiceRegistry& registry) override {
        std::cout << "[ViewportPlugin] Registering IViewportPanel service..." << std::endl;
        registry.Register<IEditorPanel>("IViewportPanel", &m_panel);
    }

    void OnInit() override {
        std::cout << "[ViewportPlugin] Viewport panel plugin initialized." << std::endl;
    }

    void OnShutdown() override {
        std::cout << "[ViewportPlugin] Viewport panel plugin shutdown." << std::endl;
    }

private:
    ViewportPanelImpl m_panel;
};

static EunoiaPluginInfo s_viewportInfo = {
    "editor-viewport",
    "1.0.0",
    "Editor 3D Viewport rendering and interaction panel",
    "Eunoia Team",
    EUNOIA_PLUGIN_API_VERSION,
    EunoiaPluginCategory::Editor,
    false // dynamic tier
};

EUNOIA_DECLARE_PLUGIN(ViewportPlugin, &s_viewportInfo)
