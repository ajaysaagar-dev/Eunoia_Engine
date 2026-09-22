#pragma once
// ============================================================================
// Eunoia Engine — IEditorPanel Interface
// ============================================================================
// Standard interface for all editor panel plugins (Outliner, Details,
// Content Browser, Viewport, World Settings, Console, etc.)
// ============================================================================

class IEditorPanel {
public:
    virtual ~IEditorPanel() = default;
    virtual const char* GetPanelName() const = 0;
    virtual void Draw() = 0;
    virtual bool IsOpen() const = 0;
    virtual void SetOpen(bool open) = 0;
};
