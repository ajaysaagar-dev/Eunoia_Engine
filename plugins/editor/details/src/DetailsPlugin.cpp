// ============================================================================
// Eunoia Engine — Details Editor Panel Plugin
// ============================================================================

#include "IEditorPanel.h"
#include "PluginAPI.h"
#include "IPlugin.h"
#include "ServiceRegistry.h"

#include <iostream>

class DetailsPanelImpl : public IEditorPanel {
public:
    const char* GetPanelName() const override { return "Details"; }
    void Draw() override {}
    bool IsOpen() const override { return m_open; }
    void SetOpen(bool open) override { m_open = open; }
private:
    bool m_open = true;
};

class DetailsPlugin : public IPlugin {
public:
    explicit DetailsPlugin(const EunoiaPluginContext* /*ctx*/) {}
    ~DetailsPlugin() override = default;

    const char* GetName() const override { return "editor-details"; }
    bool SupportsHotReload() const override { return true; }

    void OnRegister(ServiceRegistry& registry) override {
        std::cout << "[DetailsPlugin] Registering IDetailsPanel service..." << std::endl;
        registry.Register<IEditorPanel>("IDetailsPanel", &m_panel);
    }

    void OnInit() override {
        std::cout << "[DetailsPlugin] Details panel plugin initialized." << std::endl;
    }

    void OnShutdown() override {
        std::cout << "[DetailsPlugin] Details panel plugin shutdown." << std::endl;
    }

private:
    DetailsPanelImpl m_panel;
};

static EunoiaPluginInfo s_detailsInfo = {
    "editor-details",
    "1.0.0",
    "Editor Details inspector panel",
    "Eunoia Team",
    EUNOIA_PLUGIN_API_VERSION,
    EunoiaPluginCategory::Editor,
    false // dynamic tier
};

EUNOIA_DECLARE_PLUGIN(DetailsPlugin, &s_detailsInfo)
