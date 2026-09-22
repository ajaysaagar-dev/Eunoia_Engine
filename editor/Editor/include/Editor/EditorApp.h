#pragma once
// ============================================================================
// Editor::EditorApp — owns the main loop
//
// Was: inline in main() in Cube.cpp.
// Listens for EngineRHI device-lost events and shows a restart-required overlay.
// ============================================================================

#include <EngineScene/Scene.h>
#include <EngineRenderer/Camera.h>
#include <Editor/EngineUI.h>
#include <functional>
#include <string>

namespace Editor {

struct EditorConfig {
    std::string windowTitle  = "Eunoia-Editor";
    int         width        = 1280;
    int         height       = 720;
    std::string projectPath  = "";
};

class EditorApp {
public:
    static EditorApp& Get();

    bool Initialize(const EditorConfig& cfg = {});
    int  Run();                        // blocks until quit; returns exit code
    void RequestShutdown();

    // Called by EngineRHI when device is lost — shows overlay, stops rendering
    void OnDeviceLost();
    bool IsDeviceLost() const { return m_deviceLost; }

    Scene&       GetScene()  { return m_scene; }
    OrbitCamera& GetCamera() { return m_camera; }
    EngineUI&    GetUI()     { return m_ui; }

private:
    EditorApp() = default;

    bool   m_initialized = false;
    bool   m_running     = false;
    bool   m_deviceLost  = false;
    Scene       m_scene;
    OrbitCamera m_camera;
    EngineUI    m_ui;
};

} // namespace Editor
