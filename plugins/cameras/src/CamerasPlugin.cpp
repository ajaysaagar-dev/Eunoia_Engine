// ============================================================================
// Eunoia Engine — Cameras Plugin Implementation
// ============================================================================

#include "Cameras/ICameraSystem.h"
#include "PluginAPI.h"
#include "IPlugin.h"
#include "ServiceRegistry.h"

#include <iostream>

class CameraSystemImpl : public ICameraSystem {
public:
    uint32_t GetCameraCount() const override { return m_cameraCount; }
    int GetActiveLevelCameraId() const override { return m_activeCameraId; }
    void SetActiveLevelCameraId(int id) override { m_activeCameraId = id; }

private:
    uint32_t m_cameraCount = 0;
    int m_activeCameraId = -1;
};

class CamerasPlugin : public IPlugin {
public:
    explicit CamerasPlugin(const EunoiaPluginContext* /*ctx*/) {}
    ~CamerasPlugin() override = default;

    const char* GetName() const override { return "cameras"; }
    bool SupportsHotReload() const override { return true; }

    void OnRegister(ServiceRegistry& registry) override {
        std::cout << "[CamerasPlugin] Registering ICameraSystem service..." << std::endl;
        registry.Register<ICameraSystem>("ICameraSystem", &m_cameraSystem);
    }

    void OnInit() override {
        std::cout << "[CamerasPlugin] Cameras plugin initialized." << std::endl;
    }

    void OnShutdown() override {
        std::cout << "[CamerasPlugin] Cameras plugin shutdown." << std::endl;
    }

private:
    CameraSystemImpl m_cameraSystem;
};

static EunoiaPluginInfo s_camerasInfo = {
    "cameras",
    "1.0.0",
    "Camera system and level camera management",
    "Eunoia Team",
    EUNOIA_PLUGIN_API_VERSION,
    EunoiaPluginCategory::Cameras,
    false // dynamic tier
};

EUNOIA_DECLARE_PLUGIN(CamerasPlugin, &s_camerasInfo)
