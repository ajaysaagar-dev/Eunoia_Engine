// ============================================================================
// Eunoia Engine — Assets Plugin Implementation
// ============================================================================

#include "Assets/IAssetService.h"
#include "PluginAPI.h"
#include "IPlugin.h"
#include "ServiceRegistry.h"

#include <iostream>

class AssetServiceImpl : public IAssetService {
public:
    bool ScanDirectory(const std::string& /*directoryPath*/) override {
        return true;
    }

    size_t GetAssetCount() const override {
        return 0;
    }
};

class AssetsPlugin : public IPlugin {
public:
    explicit AssetsPlugin(const EunoiaPluginContext* /*ctx*/) {}
    ~AssetsPlugin() override = default;

    const char* GetName() const override { return "assets"; }
    bool SupportsHotReload() const override { return true; }

    void OnRegister(ServiceRegistry& registry) override {
        std::cout << "[AssetsPlugin] Registering IAssetService service..." << std::endl;
        registry.Register<IAssetService>("IAssetService", &m_assetService);
    }

    void OnInit() override {
        std::cout << "[AssetsPlugin] Assets plugin initialized." << std::endl;
    }

    void OnShutdown() override {
        std::cout << "[AssetsPlugin] Assets plugin shutdown." << std::endl;
    }

private:
    AssetServiceImpl m_assetService;
};

static EunoiaPluginInfo s_assetsInfo = {
    "assets",
    "1.0.0",
    "Asset registry, caching, and model importers",
    "Eunoia Team",
    EUNOIA_PLUGIN_API_VERSION,
    EunoiaPluginCategory::Assets,
    false // dynamic tier
};

EUNOIA_DECLARE_PLUGIN(AssetsPlugin, &s_assetsInfo)
