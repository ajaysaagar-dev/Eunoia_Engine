#pragma once
#include <GLFW/glfw3.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <initializer_list>

#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>

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

enum class AxisType {
    Key,        // Positive/Negative keys (e.g. D/A or W/S)
    MouseDelta, // Mouse cursor movement (0=X, 1=Y)
    MouseScroll // Mouse scroll wheel
};

struct AxisBinding {
    AxisType type = AxisType::Key;
    int positiveKey = -1;
    int negativeKey = -1;
    int mouseAxis = 0; // 0 = X, 1 = Y
    float scale = 1.0f;
    float deadzone = 0.0001f;
    bool invert = false;
};

struct InputAxisConfig {
    std::string name;
    std::vector<AxisBinding> bindings;
    float sensitivity = 1.0f;
};

struct InputAxis2DConfig {
    std::string name;
    std::string xAxisName;
    std::string yAxisName;
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
        
        if (m_isFirstMouse) {
            m_mouseXPrev = m_mouseX;
            m_mouseYPrev = m_mouseY;
            m_isFirstMouse = false;
        }

        m_mouseDeltaX = m_mouseX - m_mouseXPrev;
        m_mouseDeltaY = m_mouseY - m_mouseYPrev;
        
        m_scrollDelta = m_scrollAccum;
        m_scrollAccum = 0.0;

        // Calculate continuous cursor movement values based on moving direction
        float rawDx = (float)m_mouseDeltaX;
        float rawDy = m_invertMouseY ? (float)m_mouseDeltaY : -(float)m_mouseDeltaY; // standard 3D: moving cursor up is positive Y
        m_mouseSpeed = std::hypot(rawDx, rawDy);

        if (m_mouseSpeed > 1e-4f) {
            m_mouseDirection = glm::vec2(rawDx / m_mouseSpeed, rawDy / m_mouseSpeed);
        } else {
            m_mouseDirection = glm::vec2(0.0f);
        }

        m_mouseAxis = glm::vec2(rawDx, rawDy) * m_mouseSensitivity;
        if (std::abs(m_mouseAxis.x) < m_mouseDeadzone) m_mouseAxis.x = 0.0f;
        if (std::abs(m_mouseAxis.y) < m_mouseDeadzone) m_mouseAxis.y = 0.0f;
    }

    void ResetMouseDelta() {
        m_mouseXPrev = m_mouseX;
        m_mouseYPrev = m_mouseY;
        m_mouseDeltaX = 0.0;
        m_mouseDeltaY = 0.0;
        m_mouseAxis = glm::vec2(0.0f);
        m_mouseDirection = glm::vec2(0.0f);
        m_mouseSpeed = 0.0f;
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
    glm::vec2 GetMouseDelta() const { return glm::vec2((float)m_mouseDeltaX, (float)m_mouseDeltaY); }
    double GetScrollDelta() const { return m_scrollDelta; }

    // --- Axis System: Continuous Cursor Movement ---
    // Returns 2D cursor movement axis based on moving direction and speed (X: +Right/-Left, Y: +Up/-Down)
    glm::vec2 GetMouseAxis() const { return m_mouseAxis; }
    float GetMouseAxisX() const { return m_mouseAxis.x; }
    float GetMouseAxisY() const { return m_mouseAxis.y; }

    // Returns normalized unit vector in the direction the cursor is currently moving
    glm::vec2 GetMouseDirection() const { return m_mouseDirection; }
    float GetMouseSpeed() const { return m_mouseSpeed; }

    void SetMouseSensitivity(float sens) { m_mouseSensitivity = sens; }
    float GetMouseSensitivity() const { return m_mouseSensitivity; }

    void SetInvertMouseY(bool invert) { m_invertMouseY = invert; }
    bool GetInvertMouseY() const { return m_invertMouseY; }

    void SetMouseDeadzone(float deadzone) { m_mouseDeadzone = deadzone; }
    float GetMouseDeadzone() const { return m_mouseDeadzone; }

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

    // --- Axis Mapping System ---
    void MapAxis(const std::string& name, int positiveKey, int negativeKey, float scale = 1.0f) {
        AxisBinding b;
        b.type = AxisType::Key;
        b.positiveKey = positiveKey;
        b.negativeKey = negativeKey;
        b.scale = scale;
        m_axes[name].name = name;
        m_axes[name].bindings.push_back(b);
    }

    void MapMouseAxis(const std::string& name, int mouseDimension /* 0=X, 1=Y */, float scale = 1.0f, bool invert = false) {
        AxisBinding b;
        b.type = AxisType::MouseDelta;
        b.mouseAxis = mouseDimension;
        b.scale = scale;
        b.invert = invert;
        m_axes[name].name = name;
        m_axes[name].bindings.push_back(b);
    }

    void MapAxis(const std::string& name, const AxisBinding& binding) {
        m_axes[name].name = name;
        m_axes[name].bindings.push_back(binding);
    }

    void MapAxis2D(const std::string& name, const std::string& xAxisName, const std::string& yAxisName) {
        m_axes2D[name] = { name, xAxisName, yAxisName };
    }

    // Returns the continuous value of a 1D axis
    float GetAxis(const std::string& axisName) const {
        // Built-in mouse axis names
        if (axisName == "Mouse X" || axisName == "MouseX") {
            return GetMouseAxisX();
        }
        if (axisName == "Mouse Y" || axisName == "MouseY") {
            return GetMouseAxisY();
        }
        if (axisName == "MouseScroll" || axisName == "Scroll") {
            return (float)m_scrollDelta;
        }

        auto it = m_axes.find(axisName);
        if (it != m_axes.end()) {
            float totalValue = 0.0f;
            for (const auto& b : it->second.bindings) {
                if (b.type == AxisType::Key) {
                    float keyVal = 0.0f;
                    if (b.positiveKey >= 0 && IsKeyDown(b.positiveKey)) keyVal += 1.0f;
                    if (b.negativeKey >= 0 && IsKeyDown(b.negativeKey)) keyVal -= 1.0f;
                    totalValue += keyVal * b.scale;
                } else if (b.type == AxisType::MouseDelta) {
                    float raw = (b.mouseAxis == 0) ? (float)m_mouseDeltaX : (b.invert ? (float)m_mouseDeltaY : -(float)m_mouseDeltaY);
                    float val = raw * m_mouseSensitivity * b.scale;
                    if (std::abs(val) >= b.deadzone) {
                        totalValue += val;
                    }
                } else if (b.type == AxisType::MouseScroll) {
                    totalValue += (float)m_scrollDelta * b.scale;
                }
            }
            return std::clamp(totalValue * it->second.sensitivity, -100.0f, 100.0f);
        }

        // Default fallbacks for common names if not explicitly mapped
        if (axisName == "Horizontal") {
            float val = 0.0f;
            if (IsKeyDown(Key::D) || IsKeyDown(Key::Right)) val += 1.0f;
            if (IsKeyDown(Key::A) || IsKeyDown(Key::Left)) val -= 1.0f;
            return val;
        }
        if (axisName == "Vertical") {
            float val = 0.0f;
            if (IsKeyDown(Key::W) || IsKeyDown(Key::Up)) val += 1.0f;
            if (IsKeyDown(Key::S) || IsKeyDown(Key::Down)) val -= 1.0f;
            return val;
        }
        if (axisName == "Elevation") {
            float val = 0.0f;
            if (IsKeyDown(Key::E)) val += 1.0f;
            if (IsKeyDown(Key::Q)) val -= 1.0f;
            return val;
        }

        return 0.0f;
    }

    float GetAxisRaw(const std::string& axisName) const {
        return GetAxis(axisName);
    }

    // Returns a 2D axis value (combining X and Y)
    glm::vec2 GetAxis2D(const std::string& axis2DName) const {
        if (axis2DName == "Mouse" || axis2DName == "MouseDelta" || axis2DName == "Look") {
            return GetMouseAxis();
        }
        auto it = m_axes2D.find(axis2DName);
        if (it != m_axes2D.end()) {
            return glm::vec2(GetAxis(it->second.xAxisName), GetAxis(it->second.yAxisName));
        }
        if (axis2DName == "Move" || axis2DName == "Movement") {
            return glm::vec2(GetAxis("Horizontal"), GetAxis("Vertical"));
        }
        return glm::vec2(GetAxis(axis2DName + " X"), GetAxis(axis2DName + " Y"));
    }

    glm::vec2 GetAxis2D(const std::string& xAxisName, const std::string& yAxisName) const {
        return glm::vec2(GetAxis(xAxisName), GetAxis(yAxisName));
    }

    // --- Default Action & Axis Setup ---
    void SetupDefaultActions() {
        // Actions
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

        // Default Axes
        m_axes.clear();
        m_axes2D.clear();

        MapAxis("Horizontal", Key::D, Key::A);
        MapAxis("Vertical", Key::W, Key::S);
        MapAxis("Elevation", Key::E, Key::Q);
        MapMouseAxis("Mouse X", 0, 1.0f);
        MapMouseAxis("Mouse Y", 1, 1.0f);

        MapAxis2D("Move", "Horizontal", "Vertical");
        MapAxis2D("Movement", "Horizontal", "Vertical");
        MapAxis2D("Mouse", "Mouse X", "Mouse Y");
        MapAxis2D("Look", "Mouse X", "Mouse Y");
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
    bool m_isFirstMouse = true;

    // Axis system fields
    float m_mouseSensitivity = 0.15f;
    float m_mouseDeadzone = 0.0001f;
    bool m_invertMouseY = false;
    glm::vec2 m_mouseAxis{0.0f, 0.0f};
    glm::vec2 m_mouseDirection{0.0f, 0.0f};
    float m_mouseSpeed = 0.0f;
    
    std::unordered_map<std::string, InputBinding> m_actions;
    std::unordered_map<std::string, InputAxisConfig> m_axes;
    std::unordered_map<std::string, InputAxis2DConfig> m_axes2D;
    
    static InputSystem* s_scrollInstance; // For scroll callback
};

inline InputSystem* InputSystem::s_scrollInstance = nullptr;

