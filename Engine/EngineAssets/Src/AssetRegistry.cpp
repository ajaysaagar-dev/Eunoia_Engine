#include <EngineAssets/AssetRegistry.h>
#include <EngineAssets/AssetEvents.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <set>
#include <algorithm>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

bool AssetRegistry::RegisterAsset(const AssetMetadata& metadata) {
    if (!metadata.id.IsValid()) return false;

    // Never register sidecar metadata files
    std::string sPath = metadata.sourcePath;
    std::transform(sPath.begin(), sPath.end(), sPath.begin(), ::tolower);
    std::string vPath = metadata.virtualPath;
    std::transform(vPath.begin(), vPath.end(), vPath.begin(), ::tolower);
    std::string oName = metadata.objectName;
    std::transform(oName.begin(), oName.end(), oName.begin(), ::tolower);
    if (sPath.find(".assetmeta") != std::string::npos || sPath.find(".meta") != std::string::npos ||
        vPath.find(".assetmeta") != std::string::npos || vPath.find(".meta") != std::string::npos ||
        oName.find(".assetmeta") != std::string::npos || oName.find(".meta") != std::string::npos) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_registryMutex);

    std::string normPath = AssetPath::Normalize(metadata.virtualPath);

    AssetMetadata meta = metadata;
    meta.virtualPath = normPath;
    if (meta.objectName.empty()) {
        meta.objectName = AssetPath::GetObjectName(normPath);
    }

    m_assetsByID[meta.id] = meta;
    m_assetByVirtualPath[meta.virtualPath] = meta.id;

    auto& typeList = m_assetsByType[meta.type];
    if (std::find(typeList.begin(), typeList.end(), meta.id) == typeList.end()) {
        typeList.push_back(meta.id);
    }

    // Register dependencies
    for (const auto& depId : meta.dependencies) {
        if (depId.IsValid()) {
            auto& deps = m_dependencyGraph[meta.id];
            if (std::find(deps.begin(), deps.end(), depId) == deps.end()) {
                deps.push_back(depId);
            }
            auto& refs = m_referenceGraph[depId];
            if (std::find(refs.begin(), refs.end(), meta.id) == refs.end()) {
                refs.push_back(meta.id);
            }
        }
    }

    AssetEvents::BroadcastRegistered(meta.id);
    return true;
}

bool AssetRegistry::UnregisterAsset(const AssetID& id) {
    if (!id.IsValid()) return false;
    std::lock_guard<std::mutex> lock(m_registryMutex);

    auto it = m_assetsByID.find(id);
    if (it == m_assetsByID.end()) return false;

    AssetMetadata meta = it->second;

    m_assetByVirtualPath.erase(meta.virtualPath);

    auto typeIt = m_assetsByType.find(meta.type);
    if (typeIt != m_assetsByType.end()) {
        auto& list = typeIt->second;
        list.erase(std::remove(list.begin(), list.end(), id), list.end());
    }

    // Remove from dependency & reference graphs
    for (const auto& depId : meta.dependencies) {
        auto& refs = m_referenceGraph[depId];
        refs.erase(std::remove(refs.begin(), refs.end(), id), refs.end());
    }
    m_dependencyGraph.erase(id);

    m_assetsByID.erase(it);

    AssetEvents::BroadcastUnregistered(id);
    return true;
}

AssetMetadata* AssetRegistry::Find(const AssetID& id) {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    auto it = m_assetsByID.find(id);
    if (it != m_assetsByID.end()) return &it->second;
    return nullptr;
}

const AssetMetadata* AssetRegistry::Find(const AssetID& id) const {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    auto it = m_assetsByID.find(id);
    if (it != m_assetsByID.end()) return &it->second;
    return nullptr;
}

AssetMetadata* AssetRegistry::FindByPath(const std::string& virtualPath) {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    std::string norm = AssetPath::Normalize(virtualPath);
    auto it = m_assetByVirtualPath.find(norm);
    if (it != m_assetByVirtualPath.end()) {
        return &m_assetsByID[it->second];
    }
    return nullptr;
}

AssetMetadata* AssetRegistry::FindByNameOrPath(const std::string& query) {
    if (query.empty()) return nullptr;
    std::lock_guard<std::mutex> lock(m_registryMutex);

    // 1. Try direct virtual path
    std::string norm = AssetPath::Normalize(query);
    auto itPath = m_assetByVirtualPath.find(norm);
    if (itPath != m_assetByVirtualPath.end()) {
        return &m_assetsByID[itPath->second];
    }

    // 2. Try AssetID string
    AssetID parsedId = AssetID::FromString(query);
    if (parsedId.IsValid()) {
        auto itId = m_assetsByID.find(parsedId);
        if (itId != m_assetsByID.end()) return &itId->second;
    }

    // 3. Search by objectName or filename match
    std::string qLower = query;
    std::transform(qLower.begin(), qLower.end(), qLower.begin(), ::tolower);

    for (auto& pair : m_assetsByID) {
        std::string nameLower = pair.second.objectName;
        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
        if (nameLower == qLower) {
            return &pair.second;
        }

        std::string srcLower = std::filesystem::path(pair.second.sourcePath).filename().string();
        std::transform(srcLower.begin(), srcLower.end(), srcLower.begin(), ::tolower);
        if (srcLower == qLower) {
            return &pair.second;
        }
    }
    return nullptr;
}

std::vector<AssetMetadata> AssetRegistry::FindByType(AssetType type) const {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    std::vector<AssetMetadata> result;
    auto it = m_assetsByType.find(type);
    if (it != m_assetsByType.end()) {
        result.reserve(it->second.size());
        for (const auto& id : it->second) {
            auto assetIt = m_assetsByID.find(id);
            if (assetIt != m_assetsByID.end()) {
                result.push_back(assetIt->second);
            }
        }
    }
    return result;
}

std::vector<AssetMetadata> AssetRegistry::FindInDirectory(const std::string& virtualDirectory, bool recursive) const {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    std::vector<AssetMetadata> result;
    std::string targetDir = AssetPath::Normalize(virtualDirectory);

    for (const auto& pair : m_assetsByID) {
        std::string assetDir = AssetPath::GetDirectory(pair.second.virtualPath);
        if (recursive) {
            if (AssetPath::IsSubdirectory(targetDir, assetDir)) {
                result.push_back(pair.second);
            }
        } else {
            if (assetDir == targetDir) {
                result.push_back(pair.second);
            }
        }
    }
    return result;
}

std::vector<std::string> AssetRegistry::GetSubdirectories(const std::string& virtualDirectory) const {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    std::set<std::string> subdirs;
    std::string targetDir = AssetPath::Normalize(virtualDirectory);

    // 1. From registered assets
    for (const auto& pair : m_assetsByID) {
        std::string assetDir = AssetPath::GetDirectory(pair.second.virtualPath);
        if (assetDir != targetDir && AssetPath::IsSubdirectory(targetDir, assetDir)) {
            // Extract the immediate child directory component
            std::string sub = assetDir.substr(targetDir.length());
            if (!sub.empty() && sub[0] == '/') sub.erase(0, 1);
            auto slashPos = sub.find('/');
            std::string immediateChild = (slashPos != std::string::npos) ? sub.substr(0, slashPos) : sub;
            if (!immediateChild.empty()) {
                subdirs.insert(AssetPath::Combine(targetDir, immediateChild));
            }
        }
    }

    // 2. Also from physical disk directories under project root
    if (!m_projectRoot.empty()) {
        std::error_code ec;
        std::string diskSub = targetDir;
        if (diskSub.rfind("/Game", 0) == 0) diskSub = diskSub.substr(5);
        if (!diskSub.empty() && diskSub[0] == '/') diskSub.erase(0, 1);
        std::filesystem::path physicalDir = diskSub.empty() ? m_projectRoot : (m_projectRoot / diskSub);
        if (std::filesystem::exists(physicalDir, ec) && std::filesystem::is_directory(physicalDir, ec)) {
            for (const auto& entry : std::filesystem::directory_iterator(physicalDir, ec)) {
                if (entry.is_directory(ec)) {
                    std::string name = entry.path().filename().string();
                    if (name != "Registry" && name != ".git" && name != "Intermediate" && name != "Saved") {
                        subdirs.insert(AssetPath::Combine(targetDir, name));
                    }
                }
            }
        }
    }

    return std::vector<std::string>(subdirs.begin(), subdirs.end());
}

std::vector<AssetID> AssetRegistry::GetDependencies(const AssetID& id) const {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    auto it = m_dependencyGraph.find(id);
    if (it != m_dependencyGraph.end()) return it->second;
    return {};
}

std::vector<AssetID> AssetRegistry::GetReferences(const AssetID& id) const {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    auto it = m_referenceGraph.find(id);
    if (it != m_referenceGraph.end()) return it->second;
    return {};
}

void AssetRegistry::AddDependency(const AssetID& owner, const AssetID& dependency) {
    if (!owner.IsValid() || !dependency.IsValid()) return;
    std::lock_guard<std::mutex> lock(m_registryMutex);

    auto& deps = m_dependencyGraph[owner];
    if (std::find(deps.begin(), deps.end(), dependency) == deps.end()) {
        deps.push_back(dependency);
    }

    auto& refs = m_referenceGraph[dependency];
    if (std::find(refs.begin(), refs.end(), owner) == refs.end()) {
        refs.push_back(owner);
    }

    auto it = m_assetsByID.find(owner);
    if (it != m_assetsByID.end()) {
        auto& mdDeps = it->second.dependencies;
        if (std::find(mdDeps.begin(), mdDeps.end(), dependency) == mdDeps.end()) {
            mdDeps.push_back(dependency);
        }
    }
}

void AssetRegistry::RemoveDependency(const AssetID& owner, const AssetID& dependency) {
    if (!owner.IsValid() || !dependency.IsValid()) return;
    std::lock_guard<std::mutex> lock(m_registryMutex);

    auto itDep = m_dependencyGraph.find(owner);
    if (itDep != m_dependencyGraph.end()) {
        itDep->second.erase(std::remove(itDep->second.begin(), itDep->second.end(), dependency), itDep->second.end());
    }

    auto itRef = m_referenceGraph.find(dependency);
    if (itRef != m_referenceGraph.end()) {
        itRef->second.erase(std::remove(itRef->second.begin(), itRef->second.end(), owner), itRef->second.end());
    }
}

bool AssetRegistry::Exists(const AssetID& id) const {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    return m_assetsByID.find(id) != m_assetsByID.end();
}

bool AssetRegistry::MoveAsset(const AssetID& id, const std::string& newVirtualDirectory) {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    auto it = m_assetsByID.find(id);
    if (it == m_assetsByID.end()) return false;

    std::string oldPath = it->second.virtualPath;
    std::string newPath = AssetPath::Combine(newVirtualDirectory, it->second.objectName);

    m_assetByVirtualPath.erase(oldPath);
    it->second.virtualPath = newPath;
    m_assetByVirtualPath[newPath] = id;

    AssetEvents::BroadcastMoved(id, oldPath, newPath);
    return true;
}

bool AssetRegistry::RenameAsset(const AssetID& id, const std::string& newObjectName) {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    auto it = m_assetsByID.find(id);
    if (it == m_assetsByID.end()) return false;

    std::string oldPath = it->second.virtualPath;
    std::string parentDir = AssetPath::GetDirectory(oldPath);
    std::string newPath = AssetPath::Combine(parentDir, newObjectName);

    m_assetByVirtualPath.erase(oldPath);
    it->second.objectName = newObjectName;
    it->second.virtualPath = newPath;
    m_assetByVirtualPath[newPath] = id;

    AssetEvents::BroadcastMoved(id, oldPath, newPath);
    return true;
}

bool AssetRegistry::DeleteAsset(const AssetID& id, bool force) {
    if (!force) {
        std::vector<AssetID> refs = GetReferences(id);
        if (!refs.empty()) {
            std::cerr << "[AssetRegistry] Cannot delete asset " << id.ToString() << ": has " << refs.size() << " references.\n";
            return false;
        }
    }
    return UnregisterAsset(id);
}

void AssetRegistry::Clear() {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    m_assetsByID.clear();
    m_assetByVirtualPath.clear();
    m_assetsByType.clear();
    m_dependencyGraph.clear();
    m_referenceGraph.clear();
}

bool AssetRegistry::Save(const std::filesystem::path& registryFilePath) {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    std::error_code ec;
    std::filesystem::create_directories(registryFilePath.parent_path(), ec);

    std::ofstream file(registryFilePath);
    if (!file.is_open()) return false;

    file << "{\n  \"assets\": [\n";
    size_t count = 0;
    for (const auto& pair : m_assetsByID) {
        const auto& m = pair.second;
        file << "    {\n";
        file << "      \"assetId\": \"" << m.id.ToString() << "\",\n";
        file << "      \"type\": \"" << AssetTypeToString(m.type) << "\",\n";
        file << "      \"virtualPath\": \"" << m.virtualPath << "\",\n";
        file << "      \"objectName\": \"" << m.objectName << "\",\n";
        file << "      \"sourcePath\": \"" << m.sourcePath << "\",\n";
        file << "      \"cookedPath\": \"" << m.cookedPath << "\",\n";
        file << "      \"dependencies\": [";
        for (size_t i = 0; i < m.dependencies.size(); ++i) {
            file << "\"" << m.dependencies[i].ToString() << "\"" << (i + 1 < m.dependencies.size() ? ", " : "");
        }
        file << "]\n";
        file << "    }" << (++count < m_assetsByID.size() ? "," : "") << "\n";
    }
    file << "  ]\n}\n";
    return true;
}

bool AssetRegistry::Load(const std::filesystem::path& registryFilePath) {
    std::ifstream file(registryFilePath);
    if (!file.is_open()) return false;

    // Simple JSON parser for registry format
    std::string line;
    AssetMetadata cur;
    bool inAsset = false;

    while (std::getline(file, line)) {
        if (line.find("\"assetId\":") != std::string::npos) {
            inAsset = true;
            cur = AssetMetadata();
            auto q1 = line.find('"', line.find(':'));
            auto q2 = line.find('"', q1 + 1);
            if (q1 != std::string::npos && q2 != std::string::npos) {
                cur.id = AssetID::FromString(line.substr(q1 + 1, q2 - q1 - 1));
            }
        } else if (inAsset && line.find("\"type\":") != std::string::npos) {
            auto q1 = line.find('"', line.find(':'));
            auto q2 = line.find('"', q1 + 1);
            if (q1 != std::string::npos && q2 != std::string::npos) {
                cur.type = StringToAssetType(line.substr(q1 + 1, q2 - q1 - 1));
            }
        } else if (inAsset && line.find("\"virtualPath\":") != std::string::npos) {
            auto q1 = line.find('"', line.find(':'));
            auto q2 = line.find('"', q1 + 1);
            if (q1 != std::string::npos && q2 != std::string::npos) {
                cur.virtualPath = line.substr(q1 + 1, q2 - q1 - 1);
            }
        } else if (inAsset && line.find("\"objectName\":") != std::string::npos) {
            auto q1 = line.find('"', line.find(':'));
            auto q2 = line.find('"', q1 + 1);
            if (q1 != std::string::npos && q2 != std::string::npos) {
                cur.objectName = line.substr(q1 + 1, q2 - q1 - 1);
            }
        } else if (inAsset && line.find("\"sourcePath\":") != std::string::npos) {
            auto q1 = line.find('"', line.find(':'));
            auto q2 = line.find('"', q1 + 1);
            if (q1 != std::string::npos && q2 != std::string::npos) {
                cur.sourcePath = line.substr(q1 + 1, q2 - q1 - 1);
            }
        } else if (inAsset && line.find("\"cookedPath\":") != std::string::npos) {
            auto q1 = line.find('"', line.find(':'));
            auto q2 = line.find('"', q1 + 1);
            if (q1 != std::string::npos && q2 != std::string::npos) {
                cur.cookedPath = line.substr(q1 + 1, q2 - q1 - 1);
            }
        } else if (inAsset && line.find('}') != std::string::npos) {
            if (cur.id.IsValid()) {
                RegisterAsset(cur);
            }
            inAsset = false;
        }
    }
    return true;
}

void AssetRegistry::ReadOrGenerateSidecar(const std::filesystem::path& diskFilePath, AssetMetadata& meta) {
    std::filesystem::path metaPath = diskFilePath.string() + ".assetmeta";
    if (std::filesystem::exists(metaPath)) {
        std::ifstream mf(metaPath);
        if (mf.is_open()) {
            std::string line;
            while (std::getline(mf, line)) {
                if (line.find("assetId:") != std::string::npos) {
                    auto pos = line.find(':');
                    std::string idStr = line.substr(pos + 1);
                    while (!idStr.empty() && isspace((unsigned char)idStr.front())) idStr.erase(idStr.begin());
                    while (!idStr.empty() && isspace((unsigned char)idStr.back())) idStr.pop_back();
                    meta.id = AssetID::FromString(idStr);
                }
            }
        }
    }

    if (!meta.id.IsValid()) {
        meta.id = AssetID::CreateRandom();
        WriteSidecar(diskFilePath, meta);
    }
}

void AssetRegistry::WriteSidecar(const std::filesystem::path& diskFilePath, const AssetMetadata& meta) {
    std::filesystem::path metaPath = diskFilePath.string() + ".assetmeta";
    std::ofstream mf(metaPath);
    if (mf.is_open()) {
        mf << "# Eunoia-Editor Asset Sidecar Metadata\n";
        mf << "assetId: " << meta.id.ToString() << "\n";
        mf << "type: " << AssetTypeToString(meta.type) << "\n";
        mf << "virtualPath: " << meta.virtualPath << "\n";
    }
#ifdef _WIN32
    SetFileAttributesW(metaPath.wstring().c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif
}

void AssetRegistry::ScanAndSync(const std::filesystem::path& projectRoot, AssetRegistryProgressFn onProgress) {
    std::error_code ec;
    if (!std::filesystem::exists(projectRoot, ec)) return;
    m_projectRoot = projectRoot;

    if (onProgress) onProgress(0.1f, "Scanning project directory tree...", projectRoot.string());

    size_t scannedCount = 0;
    auto options = std::filesystem::directory_options::skip_permission_denied;
    for (auto it = std::filesystem::recursive_directory_iterator(projectRoot, options, ec);
         !ec && it != std::filesystem::recursive_directory_iterator();
         it.increment(ec)) {
        if (!it->is_directory(ec)) {
            std::string filename = it->path().filename().string();
            std::string fLower = filename;
            std::transform(fLower.begin(), fLower.end(), fLower.begin(), ::tolower);
            if (fLower.find(".assetmeta") != std::string::npos ||
                fLower.find(".meta") != std::string::npos ||
                fLower.find("assetregistry.json") != std::string::npos) {
                continue;
            }

            std::string ext = it->path().extension().string();
            AssetType type = DetectAssetTypeFromExtension(ext);
            if (type == AssetType::Unknown) continue;

            AssetMetadata meta;
            meta.type = type;
            meta.objectName = it->path().stem().string();

            std::filesystem::path rel = std::filesystem::relative(it->path(), projectRoot, ec);
            meta.sourcePath = ec ? filename : rel.generic_string();
            meta.virtualPath = AssetPath::FromDiskPath(projectRoot, it->path());

            try {
                meta.sourceSize = (uint64_t)std::filesystem::file_size(it->path(), ec);
                auto ftime = std::filesystem::last_write_time(it->path(), ec);
                meta.sourceTimestamp = (uint64_t)ftime.time_since_epoch().count();
            } catch (...) {}

            // Stable identity via sidecar metadata
            ReadOrGenerateSidecar(it->path(), meta);

            // Register asset
            RegisterAsset(meta);
            scannedCount++;

            if (onProgress && (scannedCount % 5 == 0 || scannedCount <= 5)) {
                onProgress(0.1f + 0.55f * (float)(scannedCount % 30) / 30.0f,
                           "Indexing asset #" + std::to_string(scannedCount) + ": " + meta.objectName,
                           meta.virtualPath);
            }
        }
    }

    if (onProgress) onProgress(0.70f, "Resolving material texture dependencies...", std::to_string(scannedCount) + " assets indexed");

    // Now resolve dependencies for materials
    for (auto& pair : m_assetsByID) {
        if (pair.second.type == AssetType::Material) {
            std::filesystem::path fullMatPath = projectRoot / pair.second.sourcePath;
            if (std::filesystem::exists(fullMatPath, ec)) {
                std::ifstream mf(fullMatPath);
                std::string line;
                while (std::getline(mf, line)) {
                    if (line.find("Texture:") != std::string::npos) {
                        auto colon = line.find(':');
                        if (colon != std::string::npos) {
                            std::string texVal = line.substr(colon + 1);
                            while (!texVal.empty() && isspace((unsigned char)texVal.front())) texVal.erase(texVal.begin());
                            while (!texVal.empty() && isspace((unsigned char)texVal.back())) texVal.pop_back();

                            if (!texVal.empty() && texVal != "none") {
                                AssetMetadata* texMeta = FindByNameOrPath(texVal);
                                if (texMeta) {
                                    AddDependency(pair.second.id, texMeta->id);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (onProgress) onProgress(0.90f, "Saving persistent AssetRegistry.json...", projectRoot.string());

    // Save persistent registry
    std::filesystem::path regPath = projectRoot / "Registry" / "AssetRegistry.json";
    Save(regPath);

    if (onProgress) {
        onProgress(1.0f, "Asset Registry synchronized (" + std::to_string(m_assetsByID.size()) + " assets)", regPath.string());
    }
}
