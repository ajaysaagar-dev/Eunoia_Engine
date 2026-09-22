#include <EngineAssets/AssetManager.h>
#include <EngineAssets/AssetEvents.h>
#include <iostream>
#include <fstream>

AssetManager::AssetManager() {
    InitFallbackCheckerTexture();
}

void AssetManager::Initialize(const std::filesystem::path& projectRoot) {
    m_projectRoot = projectRoot;
    AssetRegistry::Get().ScanAndSync(projectRoot);
}

void AssetManager::InitFallbackCheckerTexture() {
    const int size = 32;
    const int checkSize = 4;
    m_missingTextureFallback.width = size;
    m_missingTextureFallback.height = size;
    m_missingTextureFallback.channels = 4;
    m_missingTextureFallback.valid = true;
    m_missingTextureFallback.averageColor = glm::vec3(0.5f, 0.0f, 0.5f);
    m_missingTextureFallback.data.resize(size * size * 4);

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            bool isMagenta = ((x / checkSize) + (y / checkSize)) % 2 == 0;
            int idx = (y * size + x) * 4;
            if (isMagenta) {
                m_missingTextureFallback.data[idx + 0] = 255; // R
                m_missingTextureFallback.data[idx + 1] = 0;   // G
                m_missingTextureFallback.data[idx + 2] = 255; // B
                m_missingTextureFallback.data[idx + 3] = 255; // A
            } else {
                m_missingTextureFallback.data[idx + 0] = 0;   // R
                m_missingTextureFallback.data[idx + 1] = 0;   // G
                m_missingTextureFallback.data[idx + 2] = 0;   // B
                m_missingTextureFallback.data[idx + 3] = 255; // A
            }
        }
    }
}

CachedTexture* AssetManager::GetMissingTextureFallback() {
    return &m_missingTextureFallback;
}

AssetHandle<CachedTexture> AssetManager::GetOrLoadTexture(const AssetID& id) {
    if (!id.IsValid()) {
        return AssetHandle<CachedTexture>(id, &m_missingTextureFallback);
    }

    std::lock_guard<std::mutex> lock(m_managerMutex);

    // 1. Check runtime cache
    auto it = m_loadedTextures.find(id);
    if (it != m_loadedTextures.end()) {
        return AssetHandle<CachedTexture>(id, &it->second);
    }

    // 2. Query registry metadata
    AssetMetadata* meta = AssetRegistry::Get().Find(id);
    if (!meta) {
        m_states[id] = AssetState::Missing;
        return AssetHandle<CachedTexture>(id, &m_missingTextureFallback);
    }

    // 3. Resolve disk source path
    std::filesystem::path fullPath = m_projectRoot / meta->sourcePath;
    std::error_code ec;
    if (!std::filesystem::exists(fullPath, ec)) {
        meta->isMissing = true;
        m_states[id] = AssetState::Missing;
        std::cerr << "[AssetManager] Texture missing on disk: " << meta->sourcePath << " (AssetID: " << id.ToString() << ")\n";
        return AssetHandle<CachedTexture>(id, &m_missingTextureFallback);
    }

    // 4. Load via TextureManager
    m_states[id] = AssetState::Loading;
    CachedTexture* tex = TextureManager::Get().GetTexture(fullPath.string());
    if (!tex || !tex->valid) {
        m_states[id] = AssetState::Failed;
        return AssetHandle<CachedTexture>(id, &m_missingTextureFallback);
    }

    m_loadedTextures[id] = *tex;
    m_states[id] = AssetState::Loaded;
    return AssetHandle<CachedTexture>(id, &m_loadedTextures[id]);
}

AssetHandle<CachedTexture> AssetManager::GetOrLoadTexture(const std::string& virtualPathOrDisk) {
    if (virtualPathOrDisk.empty() || virtualPathOrDisk == "none") {
        return AssetHandle<CachedTexture>(AssetID::Null(), nullptr);
    }

    // 1. Query registry
    AssetMetadata* meta = AssetRegistry::Get().FindByNameOrPath(virtualPathOrDisk);
    if (meta) {
        return GetOrLoadTexture(meta->id);
    }

    // 2. Fallback direct disk query via TextureManager
    CachedTexture* directTex = TextureManager::Get().GetTexture(virtualPathOrDisk);
    if (directTex && directTex->valid) {
        // Register ad-hoc texture metadata
        AssetMetadata adHoc;
        adHoc.id = AssetID::CreateRandom();
        adHoc.type = AssetType::Texture;
        adHoc.virtualPath = AssetPath::Normalize("/Game/Textures/" + std::filesystem::path(virtualPathOrDisk).stem().string());
        adHoc.objectName = std::filesystem::path(virtualPathOrDisk).stem().string();
        adHoc.sourcePath = virtualPathOrDisk;
        AssetRegistry::Get().RegisterAsset(adHoc);

        std::lock_guard<std::mutex> lock(m_managerMutex);
        m_loadedTextures[adHoc.id] = *directTex;
        m_states[adHoc.id] = AssetState::Loaded;
        return AssetHandle<CachedTexture>(adHoc.id, &m_loadedTextures[adHoc.id]);
    }

    return AssetHandle<CachedTexture>(AssetID::Null(), &m_missingTextureFallback);
}

void AssetManager::Unload(const AssetID& id) {
    std::lock_guard<std::mutex> lock(m_managerMutex);
    m_loadedTextures.erase(id);
    m_states[id] = AssetState::Unloaded;
}

bool AssetManager::IsLoaded(const AssetID& id) const {
    std::lock_guard<std::mutex> lock(m_managerMutex);
    return m_loadedTextures.find(id) != m_loadedTextures.end();
}

AssetState AssetManager::GetState(const AssetID& id) const {
    std::lock_guard<std::mutex> lock(m_managerMutex);
    auto it = m_states.find(id);
    if (it != m_states.end()) return it->second;
    return AssetState::Unloaded;
}

void AssetManager::Clear() {
    std::lock_guard<std::mutex> lock(m_managerMutex);
    m_loadedTextures.clear();
    m_states.clear();
}

bool AssetManager::CookProject(const std::filesystem::path& outputDir, std::string& outLog, AssetCookProgressFn onProgress) {
    std::stringstream log;
    log << "=== COOKING PROJECT ASSETS ===\n";
    log << "Target Directory: " << outputDir.string() << "\n";

    if (onProgress) onProgress(0.05f, "Initializing target output directory...", outputDir.string());

    std::error_code ec;
    std::filesystem::path contentDir = outputDir / "Content";
    std::filesystem::create_directories(contentDir, ec);
    if (ec) {
        log << "[ERROR] Failed to create cooked Content directory: " << ec.message() << "\n";
        outLog = log.str();
        if (onProgress) onProgress(1.0f, "Error creating output directory: " + ec.message(), "");
        return false;
    }

    const auto& allAssets = AssetRegistry::Get().GetAllAssets();
    size_t cookedCount = 0;
    size_t skippedCount = 0;
    size_t totalAssets = allAssets.size();
    size_t currentIndex = 0;

    std::filesystem::path cookedRegistryPath = outputDir / "CookedAssetRegistry.json";
    std::ofstream regFile(cookedRegistryPath);
    if (!regFile.is_open()) {
        log << "[ERROR] Failed to create CookedAssetRegistry.json\n";
        outLog = log.str();
        if (onProgress) onProgress(1.0f, "Error creating CookedAssetRegistry.json", "");
        return false;
    }

    regFile << "{\n  \"cookedAssets\": [\n";
    size_t written = 0;

    for (const auto& pair : allAssets) {
        const auto& meta = pair.second;
        currentIndex++;
        float progress = 0.05f + 0.85f * ((float)currentIndex / (float)(totalAssets > 0 ? totalAssets : 1));

        if (onProgress) {
            std::string detail = "Cooking (" + std::to_string(currentIndex) + "/" + std::to_string(totalAssets) + "): " + meta.objectName;
            onProgress(progress, detail, meta.virtualPath);
        }

        std::filesystem::path src = m_projectRoot / meta.sourcePath;
        if (!std::filesystem::exists(src, ec)) {
            log << "[WARNING] Missing source for asset: " << meta.virtualPath << "\n";
            skippedCount++;
            continue;
        }

        std::string ext = src.extension().string();
        std::string cookedFilename = meta.id.ToString() + ext;
        std::filesystem::path dst = contentDir / cookedFilename;

        std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            log << "[ERROR] Failed to copy " << src.string() << " to " << dst.string() << "\n";
            skippedCount++;
            continue;
        }

        regFile << "    {\n";
        regFile << "      \"assetId\": \"" << meta.id.ToString() << "\",\n";
        regFile << "      \"type\": \"" << AssetTypeToString(meta.type) << "\",\n";
        regFile << "      \"virtualPath\": \"" << meta.virtualPath << "\",\n";
        regFile << "      \"cookedPath\": \"Content/" << cookedFilename << "\",\n";
        regFile << "      \"dependencies\": [";
        for (size_t i = 0; i < meta.dependencies.size(); ++i) {
            regFile << "\"" << meta.dependencies[i].ToString() << "\"" << (i + 1 < meta.dependencies.size() ? ", " : "");
        }
        regFile << "]\n";
        regFile << "    }" << (++written < allAssets.size() ? "," : "") << "\n";

        cookedCount++;
        log << "[COOKED] " << meta.virtualPath << " -> Content/" << cookedFilename << "\n";
    }

    if (onProgress) onProgress(0.95f, "Writing CookedAssetRegistry.json...", cookedRegistryPath.string());

    regFile << "  ]\n}\n";
    regFile.close();

    log << "\nCooking Finished Successfully!\n";
    log << "Total Assets Cooked: " << cookedCount << " | Skipped: " << skippedCount << "\n";
    log << "Cooked Registry: " << cookedRegistryPath.string() << "\n";
    outLog = log.str();

    if (onProgress) {
        onProgress(1.0f, "Cooking completed successfully! " + std::to_string(cookedCount) + " assets cooked", outputDir.string());
    }
    return true;
}
