#pragma once
#include <string>
#include <functional>

struct GLFWwindow;

class IWindow {
public:
    virtual ~IWindow() = default;
    virtual bool Create(const std::string& title, int width, int height) = 0;
    virtual void Destroy() = 0;
    virtual bool ShouldClose() const = 0;
    virtual void PollEvents() const = 0;
    virtual void SwapBuffers() = 0;
    virtual void GetFramebufferSize(int& width, int& height) const = 0;
    virtual GLFWwindow* GetNativeHandle() const = 0;
};
