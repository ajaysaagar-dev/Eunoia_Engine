#pragma once
#include <cstdint>

class ICameraSystem {
public:
    virtual ~ICameraSystem() = default;
    virtual uint32_t GetCameraCount() const = 0;
    virtual int GetActiveLevelCameraId() const = 0;
    virtual void SetActiveLevelCameraId(int id) = 0;
};
