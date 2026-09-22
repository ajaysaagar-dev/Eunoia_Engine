// ============================================================================
// Eunoia Engine — Console Editor Panel Plugin
// ============================================================================

#include "IEditorPanel.h"
#include "PluginAPI.h"
#include "IPlugin.h"
#include "ServiceRegistry.h"

#include <iostream>

class ConsolePanelImpl : public IEditorPanel {
public:
    const char* GetPanelName() const override { return "Console"; }
    void Draw() override {}
    bool IsOpen() const override { return m_open; }
    void SetOpen(bool open) override { m_open = open; }
private:
    bool m_open = true;
};

class ConsolePlugin : public IPlugin {
public:
    explicit ConsolePlugin(const EunoiaPluginContext* /*ctx*/) {}
    ~ConsolePlugin() override = default;

    const char* GetName() const override { return "editor-console"; }
    bool SupportsHotReload() const override { return true; }

    void OnRegister(ServiceRegistry& registry) override {
        std::cout << "[ConsolePlugin] Registering IConsolePanel service..." << std::endl;
        registry.Register<IEditorPanel>("IConsolePanel", &m_panel);
    }

    void OnInit() override {
        std::cout << "[ConsolePlugin] Console panel plugin initialized." << std::endl;
    }

    void OnShutdown() override {
        std::cout << "[ConsolePlugin] Console panel plugin shutdown." << std::endl;
    }

private:
    ConsolePanelImpl m_panel;
};

static EunoiaPluginInfo s_consoleInfo = {
    "editor-console",
    "1.0.0",
    "Editor Console logging and commands panel",
    "Eunoia Team",
    EUNOIA_PLUGIN_API_VERSION,
    EunoiaPluginCategory::Editor,
    false // dynamic tier
};

EUNOIA_DECLARE_PLUGIN(ConsolePlugin, &s_consoleInfo)
