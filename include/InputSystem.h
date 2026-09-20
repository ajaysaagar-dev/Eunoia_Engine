#pragma once
#include <GLFW/glfw3.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <initializer_list>

namespace Key {
    constexpr int A = GLFW_KEY_A;
    constexpr int B = GLFW_KEY_B;
    constexpr int C = GLFW_KEY_C;
    constexpr int D = GLFW_KEY_D;
    constexpr int E = GLFW_KEY_E;
    constexpr int F = GLFW_KEY_F;
    constexpr int G = GLFW_KEY_G;
    constexpr int H = GLFW_KEY_H;
    constexpr int I = GLFW_KEY_I;
    constexpr int J = GLFW_KEY_J;
    constexpr int K = GLFW_KEY_K;
    constexpr int L = GLFW_KEY_L;
    constexpr int M = GLFW_KEY_M;
    constexpr int N = GLFW_KEY_N;
    constexpr int O = GLFW_KEY_O;
    constexpr int P = GLFW_KEY_P;
    constexpr int Q = GLFW_KEY_Q;
    constexpr int R = GLFW_KEY_R;
    constexpr int S = GLFW_KEY_S;
    constexpr int T = GLFW_KEY_T;
    constexpr int U = GLFW_KEY_U;
    constexpr int V = GLFW_KEY_V;
    constexpr int W = GLFW_KEY_W;
    constexpr int X = GLFW_KEY_X;
    constexpr int Y = GLFW_KEY_Y;
    constexpr int Z = GLFW_KEY_Z;
    constexpr int Space = GLFW_KEY_SPACE;
    constexpr int Enter = GLFW_KEY_ENTER;
    constexpr int Escape = GLFW_KEY_ESCAPE;
    constexpr int Tab = GLFW_KEY_TAB;
    constexpr int LShift = GLFW_KEY_LEFT_SHIFT;
    constexpr int RShift = GLFW_KEY_RIGHT_SHIFT;
    constexpr int LCtrl = GLFW_KEY_LEFT_CONTROL;
    constexpr int RCtrl = GLFW_KEY_RIGHT_CONTROL;
    constexpr int LAlt = GLFW_KEY_LEFT_ALT;
    constexpr int RAlt = GLFW_KEY_RIGHT_ALT;
    constexpr int Left = GLFW_KEY_LEFT;
    constexpr int Right = GLFW_KEY_RIGHT;
    constexpr int Up = GLFW_KEY_UP;
    constexpr int Down = GLFW_KEY_DOWN;
    constexpr int Delete = GLFW_KEY_DELETE;
    constexpr int Backspace = GLFW_KEY_BACKSPACE;
    constexpr int F1 = GLFW_KEY_F1;
    constexpr int F2 = GLFW_KEY_F2;
    constexpr int F3 = GLFW_KEY_F3;
    constexpr int F4 = GLFW_KEY_F4;
    constexpr int F5 = GLFW_KEY_F5;
    constexpr int F6 = GLFW_KEY_F6;
    constexpr int F7 = GLFW_KEY_F7;
    constexpr int F8 = GLFW_KEY_F8;
    constexpr int F9 = GLFW_KEY_F9;
    constexpr int F10 = GLFW_KEY_F10;
    constexpr int F11 = GLFW_KEY_F11;
    constexpr int F12 = GLFW_KEY_F12;
    constexpr int Num0 = GLFW_KEY_0;
    constexpr int Num1 = GLFW_KEY_1;
    constexpr int Num2 = GLFW_KEY_2;
    constexpr int Num3 = GLFW_KEY_3;
    constexpr int Num4 = GLFW_KEY_4;
    constexpr int Num5 = GLFW_KEY_5;
    constexpr int Num6 = GLFW_KEY_6;
    constexpr int Num7 = GLFW_KEY_7;
    constexpr int Num8 = GLFW_KEY_8;
    constexpr int Num9 = GLFW_KEY_9;
}

namespace MouseButton {
    constexpr int Left = GLFW_MOUSE_BUTTON_LEFT;
    constexpr int Right = GLFW_MOUSE_BUTTON_RIGHT;
    constexpr int Middle = GLFW_MOUSE_BUTTON_MIDDLE;
}

struct InputBinding {
    std::vector<int> keys;         // Any of these keys activates the action
    std::vector<int> mouseButtons; // Any of these mouse buttons activates the action
};

class InputSystem {
public:
    static InputSystem& Get() {
        static InputSystem instance;
        return instance;
    }

    // Must be called once per frame BEFORE polling events
    void BeginFrame() {
        for (int i = 0; i < MAX_KEYS; ++i) {
            m_keyDownPrev[i] = m_keyDown[i];
        }
        for (int i = 0; i < MAX_MOUSE_BUTTONS; ++i) {
            m_mouseDownPrev[i] = m_mouseDown[i];
        }
        m_mouseXPrev = m_mouseX;
        m_mouseYPrev = m_mouseY;
        m_scrollDelta = 0.0;
    }
    
    // Must be called after glfwPollEvents each frame
    void Update(GLFWwindow* window) {
        s_scrollInstance = this;
        for (int i = 0; i < MAX_KEYS; ++i) {
            m_keyDown[i] = (glfwGetKey(window, i) == GLFW_PRESS);
        }
        for (int i = 0; i < MAX_MOUSE_BUTTONS; ++i) {
            m_mouseDown[i] = (glfwGetMouseButton(window, i) == GLFW_PRESS);
        }
        
        glfwGetCursorPos(window, &m_mouseX, &m_mouseY);
        
        m_mouseDeltaX = m_mouseX - m_mouseXPrev;
        m_mouseDeltaY = m_mouseY - m_mouseYPrev;
        
        m_scrollDelta = m_scrollAccum;
        m_scrollAccum = 0.0;
    }

    // --- Raw Key State ---
    bool IsKeyDown(int key) const {
        if (key < 0 || key >= MAX_KEYS) return false;
        return m_keyDown[key];
    }
    
    bool IsKeyPressed(int key) const {
        if (key < 0 || key >= MAX_KEYS) return false;
        return m_keyDown[key] && !m_keyDownPrev[key];
    }
    
    bool IsKeyReleased(int key) const {
        if (key < 0 || key >= MAX_KEYS) return false;
        return !m_keyDown[key] && m_keyDownPrev[key];
    }

    // --- Raw Mouse State ---
    bool IsMouseDown(int button) const {
        if (button < 0 || button >= MAX_MOUSE_BUTTONS) return false;
        return m_mouseDown[button];
    }
    
    bool IsMousePressed(int button) const {
        if (button < 0 || button >= MAX_MOUSE_BUTTONS) return false;
        return m_mouseDown[button] && !m_mouseDownPrev[button];
    }
    
    bool IsMouseReleased(int button) const {
        if (button < 0 || button >= MAX_MOUSE_BUTTONS) return false;
        return !m_mouseDown[button] && m_mouseDownPrev[button];
    }
    
    double GetMouseX() const { return m_mouseX; }
    double GetMouseY() const { return m_mouseY; }
    double GetMouseDeltaX() const { return m_mouseDeltaX; }
    double GetMouseDeltaY() const { return m_mouseDeltaY; }
    double GetScrollDelta() const { return m_scrollDelta; }

    // --- Action Mapping ---
    void MapAction(const std::string& action, const InputBinding& binding) {
        m_actions[action] = binding;
    }
    
    void MapAction(const std::string& action, int key) {
        m_actions[action] = { {key}, {} };
    }
    
    void MapActionMultiple(const std::string& action, std::initializer_list<int> keys) {
        m_actions[action] = { keys, {} };
    }
    
    bool IsActionDown(const std::string& action) const {
        auto it = m_actions.find(action);
        if (it != m_actions.end()) {
            for (int k : it->second.keys) {
                if (IsKeyDown(k)) return true;
            }
            for (int b : it->second.mouseButtons) {
                if (IsMouseDown(b)) return true;
            }
        }
        return false;
    }
    
    bool IsActionPressed(const std::string& action) const {
        auto it = m_actions.find(action);
        if (it != m_actions.end()) {
            for (int k : it->second.keys) {
                if (IsKeyPressed(k)) return true;
            }
            for (int b : it->second.mouseButtons) {
                if (IsMousePressed(b)) return true;
            }
        }
        return false;
    }
    
    bool IsActionReleased(const std::string& action) const {
        auto it = m_actions.find(action);
        if (it != m_actions.end()) {
            for (int k : it->second.keys) {
                if (IsKeyReleased(k)) return true;
            }
            for (int b : it->second.mouseButtons) {
                if (IsMouseReleased(b)) return true;
            }
        }
        return false;
    }

    // --- Default Action Setup ---
    void SetupDefaultActions() {
        MapAction("MoveForward", Key::W);
        MapAction("MoveBackward", Key::S);
        MapAction("MoveLeft", Key::A);
        MapAction("MoveRight", Key::D);
        MapAction("MoveUp", Key::E);
        MapAction("MoveDown", Key::Q);
        MapAction("Jump", Key::Space);
        MapAction("Sprint", Key::LShift);
        MapAction("Interact", Key::F);
        MapAction("Attack", InputBinding{{}, {MouseButton::Left}});
        MapAction("Zoom", InputBinding{{}, {MouseButton::Right}});
        MapAction("Select", InputBinding{{}, {MouseButton::Left}});
    }
    
    // Scroll callback (must be registered with glfwSetScrollCallback)
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        if (s_scrollInstance) {
            s_scrollInstance->m_scrollAccum += yoffset;
        }
    }

private:
    InputSystem() = default;
    
    static const int MAX_KEYS = GLFW_KEY_LAST + 1;
    static const int MAX_MOUSE_BUTTONS = GLFW_MOUSE_BUTTON_LAST + 1;
    
    bool m_keyDown[MAX_KEYS] = {};
    bool m_keyDownPrev[MAX_KEYS] = {};
    
    bool m_mouseDown[MAX_MOUSE_BUTTONS] = {};
    bool m_mouseDownPrev[MAX_MOUSE_BUTTONS] = {};
    
    double m_mouseX = 0.0, m_mouseY = 0.0;
    double m_mouseXPrev = 0.0, m_mouseYPrev = 0.0;
    double m_mouseDeltaX = 0.0, m_mouseDeltaY = 0.0;
    double m_scrollDelta = 0.0;
    double m_scrollAccum = 0.0;
    
    std::unordered_map<std::string, InputBinding> m_actions;
    
    static InputSystem* s_scrollInstance; // For scroll callback
};

inline InputSystem* InputSystem::s_scrollInstance = nullptr;
