#include "EnginePlatform/Window.h"
#include <iostream>

namespace EnginePlatform {

bool Window::Create(const WindowConfig& cfg) {
    if (!glfwInit()) {
        std::cerr << "[EnginePlatform] Failed to initialize GLFW\n";
        return false;
    }

    // Direct3D 12 does not use OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_DECORATED, cfg.decorated ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, cfg.resizable ? GLFW_TRUE : GLFW_FALSE);

    m_window = glfwCreateWindow(cfg.width, cfg.height, cfg.title.c_str(), nullptr, nullptr);
    if (!m_window) {
        std::cerr << "[EnginePlatform] Failed to create GLFW window\n";
        return false;
    }

    return true;
}

void Window::Destroy() {
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
}

bool Window::ShouldClose() const {
    return m_window ? glfwWindowShouldClose(m_window) : true;
}

void Window::PollEvents() const {
    glfwPollEvents();
}

void Window::SwapBuffers() {
    // D3D12 uses IDXGISwapChain::Present, GLFW swap buffers is a no-op with GLFW_NO_API
}

void Window::GetFramebufferSize(int& w, int& h) const {
    if (m_window) {
        glfwGetFramebufferSize(m_window, &w, &h);
    } else {
        w = 0;
        h = 0;
    }
}

void Window::SetKeyCallback(GLFWkeyfun fn) {
    if (m_window) glfwSetKeyCallback(m_window, fn);
}

void Window::SetMouseBtnCallback(GLFWmousebuttonfun fn) {
    if (m_window) glfwSetMouseButtonCallback(m_window, fn);
}

void Window::SetCursorPosCallback(GLFWcursorposfun fn) {
    if (m_window) glfwSetCursorPosCallback(m_window, fn);
}

void Window::SetScrollCallback(GLFWscrollfun fn) {
    if (m_window) glfwSetScrollCallback(m_window, fn);
}

} // namespace EnginePlatform
