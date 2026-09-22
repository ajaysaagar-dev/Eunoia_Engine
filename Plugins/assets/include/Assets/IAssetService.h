#pragma once
#include <string>

class IAssetService {
public:
    virtual ~IAssetService() = default;
    virtual bool ScanDirectory(const std::string& directoryPath) = 0;
    virtual size_t GetAssetCount() const = 0;
};
