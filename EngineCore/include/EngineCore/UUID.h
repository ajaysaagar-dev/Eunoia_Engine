#pragma once
#include <cstdint>
#include <string>
#include <random>
#include <sstream>
#include <iomanip>

// ============================================================================
// EngineCore::UUID — 128-bit random identifier
// ============================================================================

namespace EngineCore {

class UUID {
public:
    UUID() {
        // Generate random 128-bit UUID (version 4)
        std::mt19937_64 rng(std::random_device{}());
        std::uniform_int_distribution<uint64_t> dist;
        m_hi = dist(rng);
        m_lo = dist(rng);
        // Set version 4 bits
        m_hi = (m_hi & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
        m_lo = (m_lo & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;
    }

    explicit UUID(uint64_t hi, uint64_t lo) : m_hi(hi), m_lo(lo) {}

    static UUID Null() { return UUID(0, 0); }
    bool IsNull() const { return m_hi == 0 && m_lo == 0; }

    bool operator==(const UUID& other) const { return m_hi == other.m_hi && m_lo == other.m_lo; }
    bool operator!=(const UUID& other) const { return !(*this == other); }
    bool operator< (const UUID& other) const {
        return m_hi < other.m_hi || (m_hi == other.m_hi && m_lo < other.m_lo);
    }

    std::string ToString() const {
        std::ostringstream ss;
        ss << std::hex << std::setfill('0')
           << std::setw(8)  << ((m_hi >> 32) & 0xFFFFFFFF) << '-'
           << std::setw(4)  << ((m_hi >> 16) & 0xFFFF)     << '-'
           << std::setw(4)  << (m_hi & 0xFFFF)              << '-'
           << std::setw(4)  << ((m_lo >> 48) & 0xFFFF)     << '-'
           << std::setw(12) << (m_lo & 0x0000FFFFFFFFFFFFULL);
        return ss.str();
    }

    uint64_t Hi() const { return m_hi; }
    uint64_t Lo() const { return m_lo; }

private:
    uint64_t m_hi = 0;
    uint64_t m_lo = 0;
};

} // namespace EngineCore

// Hash support for use in unordered containers
namespace std {
template<>
struct hash<EngineCore::UUID> {
    size_t operator()(const EngineCore::UUID& id) const noexcept {
        return hash<uint64_t>{}(id.Hi()) ^ (hash<uint64_t>{}(id.Lo()) << 32);
    }
};
}
