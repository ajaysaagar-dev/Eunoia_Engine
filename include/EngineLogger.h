#pragma once

#include <string>
#include <deque>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <iostream>

struct LogEntry {
    std::string timestamp;
    std::string actionType;
    std::string target;
    std::string details;
};

class EngineLogger {
public:
    static EngineLogger& Get() {
        static EngineLogger instance;
        return instance;
    }

    void Init(const std::string& logFilePath = "logs.elogs") {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_filePath = logFilePath;
        FlushToFileLocked();
    }

    void LogAction(const std::string& actionType, const std::string& target = "", const std::string& details = "") {
        std::lock_guard<std::mutex> lock(m_mutex);
        LogEntry entry;
        entry.timestamp = GetCurrentTimestamp();
        entry.actionType = actionType;
        entry.target = target;
        entry.details = details;

        m_history.push_back(entry);
        while (m_history.size() > MAX_HISTORY) {
            m_history.pop_front();
        }
        FlushToFileLocked();
    }

    void LogError(const std::string& system, uint32_t code, const std::string& message, const std::string& extra = "") {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::stringstream ss;
        ss << "[" << GetCurrentTimestamp() << "] ERROR\n";
        ss << "System: " << system << "\n";
        std::stringstream hexCode;
        hexCode << "0x" << std::hex << std::uppercase << code << std::dec;
        ss << "Code: " << hexCode.str() << "\n";
        ss << "Message: " << message << "\n";
        if (!extra.empty()) {
            ss << "Details:\n" << extra << "\n";
        }
        m_lastError = ss.str();
        FlushToFileLocked();
    }

    void Flush() {
        std::lock_guard<std::mutex> lock(m_mutex);
        FlushToFileLocked();
    }

private:
    EngineLogger() : m_filePath("logs.elogs") {}
    ~EngineLogger() = default;
    EngineLogger(const EngineLogger&) = delete;
    EngineLogger& operator=(const EngineLogger&) = delete;

    static std::string GetCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
#if defined(_WIN32)
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        std::ostringstream oss;
        oss << std::setfill('0')
            << std::setw(2) << tm.tm_hour << ":"
            << std::setw(2) << tm.tm_min << ":"
            << std::setw(2) << tm.tm_sec << "."
            << std::setw(3) << ms.count();
        return oss.str();
    }

    void FlushToFileLocked() {
        std::ofstream file(m_filePath, std::ios::trunc);
        if (!file.is_open()) return;

        file << "# Eunoia-Engine Rolling Interaction History (Last " << m_history.size() << " Actions)\n\n";
        for (const auto& entry : m_history) {
            file << "[" << entry.timestamp << "] " << entry.actionType << "\n";
            if (!entry.target.empty()) {
                file << "Target: " << entry.target << "\n";
            }
            if (!entry.details.empty()) {
                file << "Details: " << entry.details << "\n";
            }
            file << "\n";
        }

        if (!m_lastError.empty()) {
            file << "================================================================================\n";
            file << m_lastError << "\n";
        }
    }

    static constexpr size_t MAX_HISTORY = 50;
    std::string m_filePath;
    std::deque<LogEntry> m_history;
    std::string m_lastError;
    std::mutex m_mutex;
};

// Global engine logging bridge (implemented in EngineUI.cpp)
void AddEngineLog(const std::string& category, const std::string& message, int level = 0);

