#pragma once
#include <string>

class IMaterialSystem {
public:
    virtual ~IMaterialSystem() = default;
    virtual bool LoadMaterial(const std::string& path) = 0;
    virtual bool SaveMaterial(const std::string& path) = 0;
};
