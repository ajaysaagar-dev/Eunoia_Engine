#pragma once
#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>
#include <random>
#include <functional>

struct AssetID {
    uint64_t high = 0;
    uint64_t low = 0;

    constexpr AssetID() = default;
    constexpr AssetID(uint64_t h, uint64_t l) : high(h), low(l) {}

    bool IsValid() const {
        return high != 0 || low != 0;
    }

    static AssetID Null() {
        return AssetID(0, 0);
    }

    static AssetID CreateRandom() {
        static thread_local std::mt19937_64 rng((uint64_t)std::random_device{}() ^ 0x9E3779B97F4A7C15ULL);
        uint64_t h = rng();
        uint64_t l = rng();
        // Set UUID version 4 (random) and RFC 4122 variant
        h = (h & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
        l = (l & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;
        return AssetID(h, l);
    }

    std::string ToString() const {
        char buf[37];
        snprintf(buf, sizeof(buf), "%08X-%04X-%04X-%04X-%012llX",
            (unsigned int)(high >> 32),
            (unsigned int)((high >> 16) & 0xFFFF),
            (unsigned int)(high & 0xFFFF),
            (unsigned int)(low >> 48),
            (unsigned long long)(low & 0xFFFFFFFFFFFFULL));
        return std::string(buf);
    }

    static AssetID FromString(const std::string& str) {
        if (str.empty() || str == "null" || str == "0" || str == "none") {
            return Null();
        }
        std::string hex;
        hex.reserve(32);
        for (char c : str) {
            if (isxdigit((unsigned char)c)) {
                hex.push_back(c);
            }
        }
        if (hex.length() < 32) {
            // Pad left or right if partial
            while (hex.length() < 32) hex.push_back('0');
        }
        try {
            uint64_t h = std::stoull(hex.substr(0, 16), nullptr, 16);
            uint64_t l = std::stoull(hex.substr(16, 16), nullptr, 16);
            return AssetID(h, l);
        } catch (...) {
            return Null();
        }
    }

    bool operator==(const AssetID& o) const {
        return high == o.high && low == o.low;
    }

    bool operator!=(const AssetID& o) const {
        return !(*this == o);
    }

    bool operator<(const AssetID& o) const {
        if (high != o.high) return high < o.high;
        return low < o.low;
    }
};

namespace std {
    template<>
    struct hash<AssetID> {
        size_t operator()(const AssetID& id) const noexcept {
            return std::hash<uint64_t>{}(id.high) ^ (std::hash<uint64_t>{}(id.low) << 1);
        }
    };
}
