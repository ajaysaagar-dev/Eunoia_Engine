#pragma once
#include <chrono>

// ============================================================================
// EngineCore::Time — high-resolution timer utilities
// ============================================================================

namespace EngineCore {

class Time {
public:
    using Clock     = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;

    static Time& Get() {
        static Time instance;
        return instance;
    }

    // Call once per frame from the main loop
    void Tick() {
        auto now   = Clock::now();
        m_deltaTime = std::chrono::duration<float>(now - m_last).count();
        m_totalTime += m_deltaTime;
        m_last = now;
    }

    float DeltaTime()  const { return m_deltaTime; }
    float TotalTime()  const { return m_totalTime; }

    // FPS based on last delta
    float FPS() const { return (m_deltaTime > 0.0f) ? (1.0f / m_deltaTime) : 0.0f; }

private:
    TimePoint m_last       = Clock::now();
    float     m_deltaTime  = 0.0f;
    float     m_totalTime  = 0.0f;
};

} // namespace EngineCore
