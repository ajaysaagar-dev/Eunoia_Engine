#pragma once
#include "AssetID.h"
#include "AssetType.h"
#include "AssetPath.h"
#include "AssetMetadata.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>
#include <mutex>

class AssetRegistry {
public:
    static AssetRegistry& Get() {
        static AssetRegistry instance;
        return instance;
    }

    bool RegisterAsset(const AssetMetadata& metadata);
    bool UnregisterAsset(const AssetID& id);

    AssetMetadata* Find(const AssetID& id);
    const AssetMetadata* Find(const AssetID& id) const;

    AssetMetadata* FindById(const AssetID& id) { return Find(id); }
    const AssetMetadata* FindById(const AssetID& id) const { return Find(id); }
    AssetMetadata* GetMetadata(const AssetID& id) { return Find(id); }
    const AssetMetadata* GetMetadata(const AssetID& id) const { return Find(id); }

    AssetMetadata* FindByPath(const std::string& virtualPath);
    AssetMetadata* FindByVirtualPath(const std::string& virtualPath) { return FindByPath(virtualPath); }
    const AssetMetadata* FindByVirtualPath(const std::string& virtualPath) const { return const_cast<AssetRegistry*>(this)->FindByPath(virtualPath); }
    AssetMetadata* FindByNameOrPath(const std::string& query);

    std::vector<AssetMetadata> FindByType(AssetType type) const;
    std::vector<AssetMetadata> FindInDirectory(const std::string& virtualDirectory, bool recursive = false) const;
    std::vector<std::string> GetSubdirectories(const std::string& virtualDirectory) const;

    void ScanDirectory(const std::filesystem::path& dir) { ScanAndSync(dir); }

    std::vector<AssetID> GetDependencies(const AssetID& id) const;
    std::vector<AssetID> GetReferences(const AssetID& id) const;

    void AddDependency(const AssetID& owner, const AssetID& dependency);
    void RemoveDependency(const AssetID& owner, const AssetID& dependency);

    bool Exists(const AssetID& id) const;

    bool MoveAsset(const AssetID& id, const std::string& newVirtualDirectory);
    bool RenameAsset(const AssetID& id, const std::string& newObjectName);
    bool DeleteAsset(const AssetID& id, bool force = false);

    bool Save(const std::filesystem::path& registryFilePath);
    bool Load(const std::filesystem::path& registryFilePath);

    using AssetRegistryProgressFn = std::function<void(float progress, const std::string& step, const std::string& detail)>;
    void ScanAndSync(const std::filesystem::path& projectRoot, AssetRegistryProgressFn onProgress = nullptr);

    size_t GetAssetCount() const { return m_assetsByID.size(); }
    const std::unordered_map<AssetID, AssetMetadata>& GetAllAssets() const { return m_assetsByID; }

    void SetProjectRoot(const std::filesystem::path& root) { m_projectRoot = root; }
    const std::filesystem::path& GetProjectRoot() const { return m_projectRoot; }

    void Clear();

private:
    AssetRegistry() = default;

    mutable std::mutex m_registryMutex;
    std::unordered_map<AssetID, AssetMetadata> m_assetsByID;
    std::unordered_map<std::string, AssetID> m_assetByVirtualPath;
    std::unordered_map<AssetType, std::vector<AssetID>> m_assetsByType;
    std::unordered_map<AssetID, std::vector<AssetID>> m_dependencyGraph;
    std::unordered_map<AssetID, std::vector<AssetID>> m_referenceGraph;
    std::filesystem::path m_projectRoot;

    void RebuildIndexes();
    void ReadOrGenerateSidecar(const std::filesystem::path& diskFilePath, AssetMetadata& meta);
    void WriteSidecar(const std::filesystem::path& diskFilePath, const AssetMetadata& meta);
};
