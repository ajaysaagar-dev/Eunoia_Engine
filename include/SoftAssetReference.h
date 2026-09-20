#pragma once
#include "AssetID.h"
#include "AssetHandle.h"
#include <string>

class SoftAssetReference {
private:
    AssetID m_id;
    std::string m_virtualPath;

public:
    SoftAssetReference() = default;
    SoftAssetReference(const AssetID& id, const std::string& virtualPath = "")
        : m_id(id), m_virtualPath(virtualPath) {}

    bool IsValid() const {
        return m_id.IsValid();
    }

    const AssetID& GetID() const {
        return m_id;
    }

    const std::string& GetVirtualPath() const {
        return m_virtualPath;
    }

    void Set(const AssetID& id, const std::string& path = "") {
        m_id = id;
        m_virtualPath = path;
    }

    void Reset() {
        m_id = AssetID::Null();
        m_virtualPath.clear();
    }
};
