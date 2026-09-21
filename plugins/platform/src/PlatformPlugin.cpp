// ============================================================================
// Eunoia Engine — Platform Plugin Implementation
// ============================================================================

#include "Platform/IWindow.h"
#include "EnginePlatform/Window.h"
#include "PluginAPI.h"
#include "IPlugin.h"
#include "ServiceRegistry.h"

#include <iostream>

class PlatformWindowService : public IWindow {
public:
    bool Create(const std::string& title, int width, int height) override {
        EnginePlatform::WindowConfig cfg;
        cfg.title = title;
        cfg.width = width;
        cfg.height = height;
        return m_window.Create(cfg);
    }

    void Destroy() override {
        m_window.Destroy();
    }

    bool ShouldClose() const override {
        return m_window.ShouldClose();
    }

    void PollEvents() const override {
        m_window.PollEvents();
    }

    void SwapBuffers() override {
        m_window.SwapBuffers();
    }

    void GetFramebufferSize(int& width, int& height) const override {
        m_window.GetFramebufferSize(width, height);
    }

    GLFWwindow* GetNativeHandle() const override {
        return m_window.Handle();
    }

private:
    EnginePlatform::Window m_window;
};

class PlatformPlugin : public IPlugin {
public:
    explicit PlatformPlugin(const EunoiaPluginContext* /*ctx*/) {}
    ~PlatformPlugin() override = default;

    const char* GetName() const override { return "platform"; }
    bool SupportsHotReload() const override { return false; } // Core plugin

    void OnRegister(ServiceRegistry& registry) override {
        std::cout << "[PlatformPlugin] Registering IWindow service..." << std::endl;
        registry.Register<IWindow>("IWindow", &m_windowService);
    }

    void OnInit() override {
        std::cout << "[PlatformPlugin] Core Platform plugin initialized." << std::endl;
    }

    void OnShutdown() override {
        std::cout << "[PlatformPlugin] Core Platform plugin shutdown." << std::endl;
        m_windowService.Destroy();
    }

private:
    PlatformWindowService m_windowService;
};

static EunoiaPluginInfo s_PlatformInfo = {
    "platform",
    "1.0.0",
    "Platform Window and Input abstraction",
    "Eunoia Team",
    EUNOIA_PLUGIN_API_VERSION,
    EunoiaPluginCategory::Platform,
    true // isCore
};

EUNOIA_DECLARE_STATIC_PLUGIN(Platform, PlatformPlugin)
