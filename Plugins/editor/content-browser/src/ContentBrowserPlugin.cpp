// ============================================================================
// Eunoia Engine — Content Browser Editor Panel Plugin
// ============================================================================

#include "IEditorPanel.h"
#include "PluginAPI.h"
#include "IPlugin.h"
#include "ServiceRegistry.h"

#include <iostream>

class ContentBrowserPanelImpl : public IEditorPanel {
public:
    const char* GetPanelName() const override { return "Content Browser"; }
    void Draw() override {}
    bool IsOpen() const override { return m_open; }
    void SetOpen(bool open) override { m_open = open; }
private:
    bool m_open = true;
};

class ContentBrowserPlugin : public IPlugin {
public:
    explicit ContentBrowserPlugin(const EunoiaPluginContext* /*ctx*/) {}
    ~ContentBrowserPlugin() override = default;

    const char* GetName() const override { return "editor-content-browser"; }
    bool SupportsHotReload() const override { return true; }

    void OnRegister(ServiceRegistry& registry) override {
        std::cout << "[ContentBrowserPlugin] Registering IContentBrowserPanel service..." << std::endl;
        registry.Register<IEditorPanel>("IContentBrowserPanel", &m_panel);
    }

    void OnInit() override {
        std::cout << "[ContentBrowserPlugin] Content Browser panel plugin initialized." << std::endl;
    }

    void OnShutdown() override {
        std::cout << "[ContentBrowserPlugin] Content Browser panel plugin shutdown." << std::endl;
    }

private:
    ContentBrowserPanelImpl m_panel;
};

static EunoiaPluginInfo s_contentBrowserInfo = {
    "editor-content-browser",
    "1.0.0",
    "Editor Content Browser asset management panel",
    "Eunoia Team",
    EUNOIA_PLUGIN_API_VERSION,
    EunoiaPluginCategory::Editor,
    false // dynamic tier
};

EUNOIA_DECLARE_PLUGIN(ContentBrowserPlugin, &s_contentBrowserInfo)
