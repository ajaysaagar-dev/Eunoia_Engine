#pragma once
#include <string>

class Scene;

class ILevelSerializer {
public:
    virtual ~ILevelSerializer() = default;
    virtual bool SaveLevel(const std::string& path, const Scene& scene) = 0;
    virtual bool LoadLevel(const std::string& path, Scene& scene) = 0;
};
