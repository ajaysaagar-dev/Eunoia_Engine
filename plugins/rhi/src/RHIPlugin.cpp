// ============================================================================
// Eunoia Engine — RHI Plugin Implementation
// ============================================================================

#include "RHI/IRHI.h"
#include "PluginAPI.h"
#include "IPlugin.h"
#include "ServiceRegistry.h"

#include <iostream>

class RHIService : public IRHI {
public:
    bool Initialize(void* /*windowHandle*/, int /*width*/, int /*height*/) override {
        // Hardware device init hooks
        return true;
    }

    void Shutdown() override {}

    ID3D12Device* GetDevice() const override { return nullptr; }
    ID3D12CommandQueue* GetCommandQueue() const override { return nullptr; }
};

class RHIPlugin : public IPlugin {
public:
    explicit RHIPlugin(const EunoiaPluginContext* /*ctx*/) {}
    ~RHIPlugin() override = default;

    const char* GetName() const override { return "rhi"; }
    bool SupportsHotReload() const override { return false; } // Core plugin

    void OnRegister(ServiceRegistry& registry) override {
        std::cout << "[RHIPlugin] Registering IRHI service..." << std::endl;
        registry.Register<IRHI>("IRHI", &m_rhiService);
    }

    void OnInit() override {
        std::cout << "[RHIPlugin] Core RHI plugin initialized." << std::endl;
    }

    void OnShutdown() override {
        std::cout << "[RHIPlugin] Core RHI plugin shutdown." << std::endl;
    }

private:
    RHIService m_rhiService;
};

static EunoiaPluginInfo s_RHIInfo = {
    "rhi",
    "1.0.0",
    "DirectX 12 Render Hardware Interface",
    "Eunoia Team",
    EUNOIA_PLUGIN_API_VERSION,
    EunoiaPluginCategory::RHI,
    true // isCore
};

EUNOIA_DECLARE_STATIC_PLUGIN(RHI, RHIPlugin)
