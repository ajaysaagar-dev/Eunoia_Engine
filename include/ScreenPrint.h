#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <mutex>
#include <algorithm>
#include <glm/glm.hpp>

namespace ScreenPrint {

struct Message {
    std::string text;
    float totalTime = 2.5f;
    float remainingTime = 2.5f;
    glm::vec4 color{0.3f, 1.0f, 0.45f, 1.0f}; // Vibrant green-cyan default
};

class System {
public:
    static System& Get() {
        static System instance;
        return instance;
    }

    void AddMessage(const std::string& text, float duration = 2.5f, const glm::vec4& color = glm::vec4(0.3f, 1.0f, 0.45f, 1.0f)) {
        std::lock_guard<std::mutex> lock(m_mutex);
        Message msg;
        msg.text = text;
        msg.totalTime = duration > 0.05f ? duration : 0.05f;
        msg.remainingTime = msg.totalTime;
        msg.color = color;
        m_messages.push_back(msg);

        // Keep maximum 25 messages on screen
        if (m_messages.size() > 25) {
            m_messages.erase(m_messages.begin());
        }
    }

    void Update(float deltaTime) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto it = m_messages.begin(); it != m_messages.end();) {
            it->remainingTime -= deltaTime;
            if (it->remainingTime <= 0.0f) {
                it = m_messages.erase(it);
            } else {
                ++it;
            }
        }
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_messages.clear();
    }

    std::vector<Message> GetMessages() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_messages;
    }

private:
    System() = default;
    std::vector<Message> m_messages;
    mutable std::mutex m_mutex;
};

namespace Detail {
    inline void Append(std::ostringstream& ss, const std::string& v) { ss << v; }
    inline void Append(std::ostringstream& ss, const char* v) { ss << (v ? v : "null"); }
    inline void Append(std::ostringstream& ss, bool v) { ss << (v ? "true" : "false"); }
    inline void Append(std::ostringstream& ss, int v) { ss << v; }
    inline void Append(std::ostringstream& ss, unsigned int v) { ss << v; }
    inline void Append(std::ostringstream& ss, long v) { ss << v; }
    inline void Append(std::ostringstream& ss, unsigned long v) { ss << v; }
    inline void Append(std::ostringstream& ss, long long v) { ss << v; }
    inline void Append(std::ostringstream& ss, unsigned long long v) { ss << v; }
    inline void Append(std::ostringstream& ss, float v) { ss << v; }
    inline void Append(std::ostringstream& ss, double v) { ss << v; }
    inline void Append(std::ostringstream& ss, const glm::vec2& v) {
        ss << "(" << v.x << ", " << v.y << ")";
    }
    inline void Append(std::ostringstream& ss, const glm::vec3& v) {
        ss << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    }
    inline void Append(std::ostringstream& ss, const glm::vec4& v) {
        ss << "(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
    }
    template<typename T>
    inline void Append(std::ostringstream& ss, T* ptr) {
        if (!ptr) ss << "null";
        else ss << (const void*)ptr;
    }

    inline void FormatHelper(std::ostringstream&) {}

    template<typename T, typename... Rest>
    inline void FormatHelper(std::ostringstream& ss, const T& first, const Rest&... rest) {
        Append(ss, first);
        FormatHelper(ss, rest...);
    }
} // namespace Detail

} // namespace ScreenPrint

// ============================================================================
// Global Print functions:
// - Print(value, time): prints value on screen top-left for specified seconds
// - Print(value): prints with default 2.5s duration
// - Print(val1, val2, ...): formats multiple values
// ============================================================================

template<typename T>
inline void Print(const T& value, float time) {
    std::ostringstream ss;
    ScreenPrint::Detail::Append(ss, value);
    ScreenPrint::System::Get().AddMessage(ss.str(), time);
}

inline void Print(const char* value, float time) {
    ScreenPrint::System::Get().AddMessage(value ? value : "null", time);
}

inline void Print(const std::string& value, float time) {
    ScreenPrint::System::Get().AddMessage(value, time);
}

template<typename T>
inline void Print(const T& value) {
    std::ostringstream ss;
    ScreenPrint::Detail::Append(ss, value);
    ScreenPrint::System::Get().AddMessage(ss.str(), 2.5f);
}

inline void Print(const char* value) {
    ScreenPrint::System::Get().AddMessage(value ? value : "null", 2.5f);
}

inline void Print(const std::string& value) {
    ScreenPrint::System::Get().AddMessage(value, 2.5f);
}

template<typename T1, typename T2, typename... Rest>
inline void Print(const T1& v1, const T2& v2, const Rest&... rest) {
    std::ostringstream ss;
    ScreenPrint::Detail::FormatHelper(ss, v1, v2, rest...);
    ScreenPrint::System::Get().AddMessage(ss.str(), 2.5f);
}
