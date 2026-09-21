#include "../../EunoiaPluginCore/PluginManager.h"
#include "../../EunoiaPluginCore/IEditorPanel.h"
#include "../../plugins/engine/include/Engine/ILog.h"
#include "../../plugins/platform/include/Platform/IWindow.h"
#include "../../plugins/rhi/include/RHI/IRHI.h"
#include "../../plugins/scene/include/Scene/ISceneManager.h"
#include "../../plugins/rendering/include/Rendering/IRenderer.h"
#include "../../plugins/primitives/include/Primitives/IPrimitiveFactory.h"
#include "../../plugins/assets/include/Assets/IAssetService.h"
#include "../../plugins/materials/include/Materials/IMaterialSystem.h"
#include "../../plugins/levels/include/Levels/ILevelSerializer.h"
#include "../../plugins/lights/include/Lights/ILightSystem.h"
#include "../../plugins/behaviours/include/Behaviours/IBehaviourService.h"

#include <iostream>
#include <cassert>

EUNOIA_DECLARE_STATIC_PLUGIN_REF(Engine)
EUNOIA_DECLARE_STATIC_PLUGIN_REF(Platform)
EUNOIA_DECLARE_STATIC_PLUGIN_REF(RHI)

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << "  Eunoia Engine — Modular Plugin Architecture Verification" << std::endl;
    std::cout << "==========================================================" << std::endl;

    PluginManager pm;

    std::cout << "\n[1] Registering Core Tier Plugins (Static)..." << std::endl;
    EUNOIA_REGISTER_STATIC_PLUGIN(pm, Engine);
    EUNOIA_REGISTER_STATIC_PLUGIN(pm, Platform);
    EUNOIA_REGISTER_STATIC_PLUGIN(pm, RHI);

    std::cout << "\n[2] Discovering & Loading Plugins from 'plugins/'..." << std::endl;
    bool loadOk = pm.LoadAll("plugins");
    assert(loadOk && "Failed to load plugins from plugins directory!");

    auto loadedNames = pm.GetLoadedPluginNames();
    std::cout << "\n[3] Topological Dependency Load Order (" << loadedNames.size() << " plugins):" << std::endl;
    for (size_t i = 0; i < loadedNames.size(); ++i) {
        const std::string& name = loadedNames[i];
        const auto* p = pm.GetPlugin(name);
        std::cout << "  " << (i + 1) << ". " << name << " (" << (p && p->isCore ? "core" : "dynamic") << ")" << std::endl;
    }

    std::cout << "\n[4] Initializing All Plugins (Phase 1: OnRegister -> Phase 2: OnInit)..." << std::endl;
    pm.InitAll();

    std::cout << "\n[5] Verifying Service Registry across Plugin Boundaries..." << std::endl;
    auto* log = pm.GetRegistry().Require<ILog>("ILog");
    log->Info("PluginTest", "Core ILog service is working.");

    assert(pm.GetRegistry().Require<IWindow>("IWindow") != nullptr);
    assert(pm.GetRegistry().Require<IRHI>("IRHI") != nullptr);
    assert(pm.GetRegistry().Require<ISceneManager>("ISceneManager") != nullptr);
    assert(pm.GetRegistry().Require<IRenderer>("IRenderer") != nullptr);
    assert(pm.GetRegistry().Require<IAssetService>("IAssetService") != nullptr);
    assert(pm.GetRegistry().Require<IMaterialSystem>("IMaterialSystem") != nullptr);
    assert(pm.GetRegistry().Require<ILevelSerializer>("ILevelSerializer") != nullptr);
    assert(pm.GetRegistry().Require<ILightSystem>("ILightSystem") != nullptr);
    assert(pm.GetRegistry().Require<IBehaviourService>("IBehaviourService") != nullptr);

    // Verify Editor Panel Plugins
    assert(pm.GetRegistry().Require<IEditorPanel>("IOutlinerPanel") != nullptr);
    assert(pm.GetRegistry().Require<IEditorPanel>("IDetailsPanel") != nullptr);
    assert(pm.GetRegistry().Require<IEditorPanel>("IContentBrowserPanel") != nullptr);
    assert(pm.GetRegistry().Require<IEditorPanel>("IViewportPanel") != nullptr);
    assert(pm.GetRegistry().Require<IEditorPanel>("IWorldSettingsPanel") != nullptr);
    assert(pm.GetRegistry().Require<IEditorPanel>("IConsolePanel") != nullptr);

    std::cout << ">> All 15 cross-plugin services successfully resolved from ServiceRegistry!" << std::endl;

    std::cout << "\n[6] Testing Procedural Mesh Service (Primitives Plugin)..." << std::endl;
    auto* primFactory = pm.GetRegistry().Require<IPrimitiveFactory>("IPrimitiveFactory");
    PrimitiveMeshData cube = primFactory->CreateCube(2.0f, 4);
    std::cout << ">> Generated Cube: " << cube.vertices.size() << " vertices, " << cube.indices.size() << " indices" << std::endl;
    assert(!cube.vertices.empty() && !cube.indices.empty());

    std::cout << "\n[7] Testing Hot Reload (Unload -> Reload -> Re-register)..." << std::endl;
    bool reloadOk = pm.ReloadPlugin("primitives");
    assert(reloadOk && "Hot reload of primitives plugin failed!");
    auto* reloadedFactory = pm.GetRegistry().Require<IPrimitiveFactory>("IPrimitiveFactory");
    PrimitiveMeshData sphere = reloadedFactory->CreateSphere(1.0f, 16, 16);
    std::cout << ">> Generated Sphere after hot reload: " << sphere.vertices.size() << " vertices" << std::endl;
    assert(!sphere.vertices.empty());

    std::cout << "\n[8] Testing Clean Reverse-Dependency Shutdown..." << std::endl;
    pm.ShutdownAll();

    std::cout << "\n==========================================================" << std::endl;
    std::cout << "  SUCCESS: All 17 plugins verified with clean architecture!" << std::endl;
    std::cout << "==========================================================" << std::endl;
    return 0;
}
