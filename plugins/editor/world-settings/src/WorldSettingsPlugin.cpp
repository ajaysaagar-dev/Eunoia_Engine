// ============================================================================
// Eunoia Engine — World Settings Editor Panel Plugin
// ============================================================================

#include "IEditorPanel.h"
#include "PluginAPI.h"
#include "IPlugin.h"
#include "ServiceRegistry.h"

#include <iostream>

class WorldSettingsPanelImpl : public IEditorPanel {
public:
    const char* GetPanelName() const override { return "World Settings"; }
    void Draw() override {}
    bool IsOpen() const override { return m_open; }
    void SetOpen(bool open) override { m_open = open; }
private:
    bool m_open = true;
};

class WorldSettingsPlugin : public IPlugin {
public:
    explicit WorldSettingsPlugin(const EunoiaPluginContext* /*ctx*/) {}
    ~WorldSettingsPlugin() override = default;

    const char* GetName() const override { return "editor-world-settings"; }
    bool SupportsHotReload() const override { return true; }

    void OnRegister(ServiceRegistry& registry) override {
        std::cout << "[WorldSettingsPlugin] Registering IWorldSettingsPanel service..." << std::endl;
        registry.Register<IEditorPanel>("IWorldSettingsPanel", &m_panel);
    }

    void OnInit() override {
        std::cout << "[WorldSettingsPlugin] World Settings panel plugin initialized." << std::endl;
    }

    void OnShutdown() override {
        std::cout << "[WorldSettingsPlugin] World Settings panel plugin shutdown." << std::endl;
    }

private:
    WorldSettingsPanelImpl m_panel;
};

static EunoiaPluginInfo s_worldSettingsInfo = {
    "editor-world-settings",
    "1.0.0",
    "Editor World Settings and environment panel",
    "Eunoia Team",
    EUNOIA_PLUGIN_API_VERSION,
    EunoiaPluginCategory::Editor,
    false // dynamic tier
};

EUNOIA_DECLARE_PLUGIN(WorldSettingsPlugin, &s_worldSettingsInfo)
