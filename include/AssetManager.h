#pragma once
#include "AssetID.h"
#include "AssetRegistry.h"
#include "AssetHandle.h"
#include "TextureManager.h"
#include <unordered_map>
#include <memory>
#include <mutex>

enum class AssetState {
    Unloaded,
    Loading,
    Loaded,
    Missing,
    Failed
};

// Forward declaration if needed
struct MaterialAsset;

class AssetManager {
public:
    static AssetManager& Get() {
        static AssetManager instance;
        return instance;
    }

    void Initialize(const std::filesystem::path& projectRoot);

    AssetHandle<CachedTexture> GetOrLoadTexture(const AssetID& id);
    AssetHandle<CachedTexture> GetOrLoadTexture(const std::string& virtualPathOrDisk);

    void Unload(const AssetID& id);
    bool IsLoaded(const AssetID& id) const;
    AssetState GetState(const AssetID& id) const;

    CachedTexture* GetMissingTextureFallback();

    // Asset Cooking & Packaging Pipeline (dev.md Section 24, 25)
    using AssetCookProgressFn = std::function<void(float progress, const std::string& currentItem, const std::string& subDetail)>;
    bool CookProject(const std::filesystem::path& outputDir, std::string& outLog, AssetCookProgressFn onProgress = nullptr);

    void Clear();

private:
    AssetManager();

    std::filesystem::path m_projectRoot;
    mutable std::mutex m_managerMutex;

    std::unordered_map<AssetID, CachedTexture> m_loadedTextures;
    std::unordered_map<AssetID, AssetState> m_states;

    CachedTexture m_missingTextureFallback;
    void InitFallbackCheckerTexture();
};
