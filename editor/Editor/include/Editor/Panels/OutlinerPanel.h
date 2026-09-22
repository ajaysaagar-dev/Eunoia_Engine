#pragma once
// ============================================================================
// Editor::OutlinerPanel — actor hierarchy panel (Left sidebar)
// Was: inline inside EngineUI::RenderOutliner() in src/EngineUI.cpp
// ============================================================================

#include "../../../include/Scene.h"
#include <string>

namespace Editor {

class OutlinerPanel {
public:
    // Renders the entire outliner window. Returns true if selection changed.
    bool Render(Scene& scene);

    // Search filter exposed for testing
    const std::string& GetSearchFilter() const { return m_searchFilter; }

private:
    std::string m_searchFilter;
    bool        m_renameActive = false;
    int         m_renameTargetId = -1;
    char        m_renameBuf[128] = {};
};

} // namespace Editor
