#pragma once
#include <string>
#include <vector>
#include <cstdio>
#include <cstdarg>

// ============================================================================
// EngineCore::Log — lightweight categorized logging
// ============================================================================

namespace EngineCore {

enum class LogLevel {
    Info    = 0,
    Warning = 1,
    Success = 2,
    Error   = 3
};

struct LogEntry {
    std::string category;
    std::string message;
    LogLevel    level = LogLevel::Info;
};

// Global log sink (can be replaced by editor UI at startup)
class Log {
public:
    static Log& Get() {
        static Log instance;
        return instance;
    }

    void Info   (const std::string& cat, const std::string& msg) { push(cat, msg, LogLevel::Info); }
    void Warning(const std::string& cat, const std::string& msg) { push(cat, msg, LogLevel::Warning); }
    void Success(const std::string& cat, const std::string& msg) { push(cat, msg, LogLevel::Success); }
    void Error  (const std::string& cat, const std::string& msg) { push(cat, msg, LogLevel::Error); }

    const std::vector<LogEntry>& Entries() const { return entries; }
    void Clear() { entries.clear(); }

private:
    std::vector<LogEntry> entries;

    void push(const std::string& cat, const std::string& msg, LogLevel lv) {
        entries.push_back({ cat, msg, lv });
        const char* prefix = (lv == LogLevel::Error) ? "[ERROR]"
                           : (lv == LogLevel::Warning) ? "[WARN]"
                           : (lv == LogLevel::Success) ? "[OK]" : "[INFO]";
        std::fprintf(stderr, "%s [%s] %s\n", prefix, cat.c_str(), msg.c_str());
    }
};

// Convenience macros
#define ECORE_LOG_INFO(cat, msg)    EngineCore::Log::Get().Info(cat, msg)
#define ECORE_LOG_WARN(cat, msg)    EngineCore::Log::Get().Warning(cat, msg)
#define ECORE_LOG_SUCCESS(cat, msg) EngineCore::Log::Get().Success(cat, msg)
#define ECORE_LOG_ERROR(cat, msg)   EngineCore::Log::Get().Error(cat, msg)

} // namespace EngineCore
