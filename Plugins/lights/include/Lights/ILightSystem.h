#pragma once
#include <cstdint>

class ILightSystem {
public:
    virtual ~ILightSystem() = default;
    virtual uint32_t GetActiveLightCount() const = 0;
};
