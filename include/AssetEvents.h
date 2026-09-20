#pragma once
#include "AssetID.h"
#include <functional>
#include <vector>

class AssetEvents {
public:
    using AssetCallback = std::function<void(const AssetID&)>;
    using AssetMoveCallback = std::function<void(const AssetID&, const std::string& /*oldPath*/, const std::string& /*newPath*/)>;

    static void SubscribeOnAssetRegistered(AssetCallback cb) {
        Get().m_onRegistered.push_back(cb);
    }
    static void SubscribeOnAssetUnregistered(AssetCallback cb) {
        Get().m_onUnregistered.push_back(cb);
    }
    static void SubscribeOnAssetMoved(AssetMoveCallback cb) {
        Get().m_onMoved.push_back(cb);
    }
    static void SubscribeOnAssetModified(AssetCallback cb) {
        Get().m_onModified.push_back(cb);
    }

    static void BroadcastRegistered(const AssetID& id) {
        for (auto& cb : Get().m_onRegistered) if (cb) cb(id);
    }
    static void BroadcastUnregistered(const AssetID& id) {
        for (auto& cb : Get().m_onUnregistered) if (cb) cb(id);
    }
    static void BroadcastMoved(const AssetID& id, const std::string& oldP, const std::string& newP) {
        for (auto& cb : Get().m_onMoved) if (cb) cb(id, oldP, newP);
    }
    static void BroadcastModified(const AssetID& id) {
        for (auto& cb : Get().m_onModified) if (cb) cb(id);
    }

private:
    static AssetEvents& Get() {
        static AssetEvents instance;
        return instance;
    }

    std::vector<AssetCallback> m_onRegistered;
    std::vector<AssetCallback> m_onUnregistered;
    std::vector<AssetMoveCallback> m_onMoved;
    std::vector<AssetCallback> m_onModified;
};
