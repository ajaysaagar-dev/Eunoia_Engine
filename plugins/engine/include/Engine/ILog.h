#pragma once
#include <string>

enum class EngineLogLevel {
    Info = 0,
    Warning = 1,
    Success = 2,
    Error = 3
};

class ILog {
public:
    virtual ~ILog() = default;
    virtual void Info(const std::string& category, const std::string& message) = 0;
    virtual void Warning(const std::string& category, const std::string& message) = 0;
    virtual void Success(const std::string& category, const std::string& message) = 0;
    virtual void Error(const std::string& category, const std::string& message) = 0;
};
