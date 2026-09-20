#pragma once
// ============================================================================
// EnginePlatform::Window — GLFW window creation and event callbacks
// The actual GLFWwindow* is owned here; platform-specific Win32 HWND is
// exposed only when GLFW_EXPOSE_NATIVE_WIN32 is defined by the includer.
// ============================================================================

#include <GLFW/glfw3.h>
#include <string>
#include <functional>

namespace EnginePlatform {

struct WindowConfig {
    std::string title  = "Eunoia-Editor";
    int         width  = 1280;
    int         height = 720;
    bool        decorated = true;   // borderless if false
    bool        resizable = true;
};

class Window {
public:
    Window() = default;
    ~Window() { Destroy(); }

    // Not copyable
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool Create(const WindowConfig& cfg);
    void Destroy();

    bool ShouldClose() const;
    void PollEvents() const;
    void SwapBuffers();
    void GetFramebufferSize(int& w, int& h) const;

    GLFWwindow* Handle() const { return m_window; }

    // Resize callback signature: (int newWidth, int newHeight)
    void SetResizeCallback(std::function<void(int,int)> cb);

    // Key / mouse callbacks — forwarded to InputSystem if connected
    void SetKeyCallback    (GLFWkeyfun fn);
    void SetMouseBtnCallback(GLFWmousebuttonfun fn);
    void SetCursorPosCallback(GLFWcursorposfun fn);
    void SetScrollCallback (GLFWscrollfun fn);

private:
    GLFWwindow* m_window = nullptr;
};

} // namespace EnginePlatform
