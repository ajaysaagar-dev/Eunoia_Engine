#pragma once
// ============================================================================
// Editor::ConsolePanel — Output Log panel
// Was: g_engineUI.AddLog() scattered through Cube.cpp and EngineUI.cpp
// ============================================================================

#include <string>
#include <vector>

namespace Editor {

struct ConsoleEntry {
    std::string category;
    std::string message;
    int         level = 0; // 0=Info, 1=Warn, 2=Success, 3=Error
};

class ConsolePanel {
public:
    static ConsolePanel& Get();

    void AddLog(const std::string& category, const std::string& message, int level = 0);
    void Clear();
    void Render();

    const std::vector<ConsoleEntry>& Entries() const { return m_entries; }

private:
    ConsolePanel() = default;
    std::vector<ConsoleEntry> m_entries;
    bool m_autoScroll = true;
    char m_filterBuf[128] = {};
};

} // namespace Editor
