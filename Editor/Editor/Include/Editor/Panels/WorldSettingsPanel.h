#pragma once
// ============================================================================
// Editor::WorldSettingsPanel — environment, shadows, and scene global settings
// Was: inline inside EngineUI::RenderWorldSettings() in src/EngineUI.cpp
// ============================================================================

#include <EngineScene/Scene.h>

namespace Editor {

class WorldSettingsPanel {
public:
    void Render(Scene& scene);

private:
    bool m_showAdvanced = false;
};

} // namespace Editor
