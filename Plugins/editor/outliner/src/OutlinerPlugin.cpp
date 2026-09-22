// ============================================================================
// Eunoia Engine — Outliner Editor Panel Plugin
// ============================================================================

#include "IEditorPanel.h"
#include "PluginAPI.h"
#include "IPlugin.h"
#include "ServiceRegistry.h"

#include <iostream>

class OutlinerPanelImpl : public IEditorPanel {
public:
    const char* GetPanelName() const override { return "Outliner"; }
    void Draw() override {}
    bool IsOpen() const override { return m_open; }
    void SetOpen(bool open) override { m_open = open; }
private:
    bool m_open = true;
};

class OutlinerPlugin : public IPlugin {
public:
    explicit OutlinerPlugin(const EunoiaPluginContext* /*ctx*/) {}
    ~OutlinerPlugin() override = default;

    const char* GetName() const override { return "editor-outliner"; }
    bool SupportsHotReload() const override { return true; }

    void OnRegister(ServiceRegistry& registry) override {
        std::cout << "[OutlinerPlugin] Registering IOutlinerPanel service..." << std::endl;
        registry.Register<IEditorPanel>("IOutlinerPanel", &m_panel);
    }

    void OnInit() override {
        std::cout << "[OutlinerPlugin] Outliner panel plugin initialized." << std::endl;
    }

    void OnShutdown() override {
        std::cout << "[OutlinerPlugin] Outliner panel plugin shutdown." << std::endl;
    }

private:
    OutlinerPanelImpl m_panel;
};

static EunoiaPluginInfo s_outlinerInfo = {
    "editor-outliner",
    "1.0.0",
    "Editor Outliner hierarchy tree panel",
    "Eunoia Team",
    EUNOIA_PLUGIN_API_VERSION,
    EunoiaPluginCategory::Editor,
    false // dynamic tier
};

EUNOIA_DECLARE_PLUGIN(OutlinerPlugin, &s_outlinerInfo)
