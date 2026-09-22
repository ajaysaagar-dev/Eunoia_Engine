#pragma once
#include "AssetID.h"
#include <cassert>

template<typename T>
class AssetHandle {
private:
    AssetID m_id;
    T* m_ptr = nullptr;

public:
    AssetHandle() = default;
    AssetHandle(const AssetID& id, T* ptr) : m_id(id), m_ptr(ptr) {}

    bool IsValid() const {
        return m_id.IsValid() && m_ptr != nullptr;
    }

    const AssetID& GetID() const {
        return m_id;
    }

    T* Get() const {
        return m_ptr;
    }

    T* operator->() const {
        return m_ptr;
    }

    T& operator*() const {
        assert(m_ptr != nullptr);
        return *m_ptr;
    }

    explicit operator bool() const {
        return IsValid();
    }
};
