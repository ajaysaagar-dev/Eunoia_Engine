#pragma once
#include <variant>
#include <string>

// ============================================================================
// EngineCore::Result<T> — typed error wrapper (replaces bare bool/HRESULT)
// Inspired by Rust's Result<T, E>.
// ============================================================================

namespace EngineCore {

struct Error {
    std::string message;
    int         code = 0;

    explicit Error(std::string msg, int c = 0)
        : message(std::move(msg)), code(c) {}
};

template<typename T>
class Result {
public:
    // Construct from a successful value
    static Result Ok(T value) {
        Result r;
        r.m_data = std::move(value);
        return r;
    }

    // Construct from an error
    static Result Err(Error err) {
        Result r;
        r.m_data = std::move(err);
        return r;
    }

    bool IsOk()  const { return std::holds_alternative<T>(m_data); }
    bool IsErr() const { return std::holds_alternative<Error>(m_data); }

    const T&     Value() const { return std::get<T>(m_data); }
          T&     Value()       { return std::get<T>(m_data); }
    const Error& GetError() const { return std::get<Error>(m_data); }

    // Implicit bool: true on success
    explicit operator bool() const { return IsOk(); }

private:
    std::variant<T, Error> m_data;
};

// Specialization for void (success/failure only)
template<>
class Result<void> {
public:
    static Result Ok()          { Result r; r.m_ok = true;  return r; }
    static Result Err(Error e)  { Result r; r.m_ok = false; r.m_err = std::move(e); return r; }

    bool IsOk()  const { return m_ok; }
    bool IsErr() const { return !m_ok; }
    const Error& GetError() const { return m_err; }

    explicit operator bool() const { return m_ok; }

private:
    bool  m_ok  = true;
    Error m_err{""};
};

} // namespace EngineCore
