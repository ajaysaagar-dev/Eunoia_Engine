#pragma once
// ============================================================================
// Editor::ContentBrowserPanel — quick-spawn assets, Content Browser (Bottom)
// Was: inline inside EngineUI::RenderContentBrowser() in src/EngineUI.cpp
// ============================================================================

#include "../../../include/Scene.h"
#include "../../../include/AssetSystem.h"
#include <string>

namespace Editor {

class ContentBrowserPanel {
public:
    void Render(Scene& scene);

    const std::string& CurrentDirectory() const { return m_currentDir; }
    void NavigateTo(const std::string& path);

private:
    std::string m_currentDir;
    std::string m_searchFilter;
};

} // namespace Editor
