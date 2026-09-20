#pragma once
// ============================================================================
// Editor::Theme — ImGui style / theme setup
// Was: EngineUI::SetupTheme() in src/EngineUI.cpp
// ============================================================================

namespace Editor {
    // Apply the Eunoia dark theme to ImGui. Call after ImGui::NewFrame is first called.
    void SetupTheme();
}
