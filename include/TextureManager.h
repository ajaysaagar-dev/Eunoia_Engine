#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <glm/glm.hpp>
extern "C" {
    unsigned char *stbi_load(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
    void stbi_image_free(void *retval_from_stbi_load);
}

struct CachedTexture {
    int width = 0;
    int height = 0;
    int channels = 0;
    std::vector<uint8_t> data;
    glm::vec3 averageColor{0.5f, 0.5f, 0.5f};
    bool valid = false;

    // Fast point/bilinear sample at wrapped UV
    glm::vec4 Sample(float u, float v) const {
        if (!valid || data.empty() || width <= 0 || height <= 0) {
            return glm::vec4(averageColor, 1.0f);
        }
        // Repeat wrapping
        u = u - std::floor(u);
        v = v - std::floor(v);
        // Direct3D / standard UV orientation
        v = 1.0f - v;

        int x = (int)(u * (width - 1));
        int y = (int)(v * (height - 1));
        if (x < 0) x = 0; else if (x >= width) x = width - 1;
        if (y < 0) y = 0; else if (y >= height) y = height - 1;

        int idx = (y * width + x) * channels;
        float r = data[idx + 0] / 255.0f;
        float g = (channels > 1) ? (data[idx + 1] / 255.0f) : r;
        float b = (channels > 2) ? (data[idx + 2] / 255.0f) : r;
        float a = (channels > 3) ? (data[idx + 3] / 255.0f) : 1.0f;
        return glm::vec4(r, g, b, a);
    }
};

class TextureManager {
private:
    std::unordered_map<std::string, CachedTexture> m_cache;
    std::unordered_map<std::string, std::string> m_resolvedPathCache;
    std::filesystem::path m_projectRoot;

    TextureManager() {
        m_projectRoot = "C:\\Projects\\Vulkan-Cube\\projects\\test";
    }

public:
    static TextureManager& Get() {
        static TextureManager instance;
        return instance;
    }

    void SetProjectRoot(const std::filesystem::path& root) {
        if (m_projectRoot != root) {
            m_projectRoot = root;
            m_resolvedPathCache.clear();
        }
    }

    void ClearCache() {
        m_cache.clear();
        m_resolvedPathCache.clear();
    }

    std::string ResolvePath(const std::string& name) {
        if (name.empty() || name == "none") return "";
        auto itCache = m_resolvedPathCache.find(name);
        if (itCache != m_resolvedPathCache.end()) {
            return itCache->second;
        }

        std::string result = "";
        std::error_code ec;

        // 1. Direct absolute or relative path
        if (std::filesystem::exists(name, ec)) {
            result = std::filesystem::canonical(name, ec).string();
            if (result.empty()) result = name;
        }
        // 2. Relative to project root
        else if (std::filesystem::exists(m_projectRoot / name, ec)) {
            result = (m_projectRoot / name).string();
        }
        // 3. Under Materials/textures/
        else if (std::filesystem::exists(m_projectRoot / "Materials" / "textures" / name, ec)) {
            result = (m_projectRoot / "Materials" / "textures" / name).string();
        }
        // 4. Under Materials/
        else if (std::filesystem::exists(m_projectRoot / "Materials" / name, ec)) {
            result = (m_projectRoot / "Materials" / name).string();
        }
        // 5. Recursive search matching filename
        else {
            std::string targetFilename = std::filesystem::path(name).filename().string();
            auto options = std::filesystem::directory_options::skip_permission_denied;
            for (auto it = std::filesystem::recursive_directory_iterator(m_projectRoot, options, ec);
                 !ec && it != std::filesystem::recursive_directory_iterator();
                 it.increment(ec)) {
                if (!it->is_directory(ec) && it->path().filename().string() == targetFilename) {
                    result = it->path().string();
                    break;
                }
            }
        }

        // Cache the result (even if empty, avoiding repeated failed disk searches every frame)
        m_resolvedPathCache[name] = result;
        return result;
    }

    CachedTexture* GetTexture(const std::string& name) {
        if (name.empty() || name == "none") return nullptr;
        std::string resolved = ResolvePath(name);
        if (resolved.empty()) return nullptr;

        auto it = m_cache.find(resolved);
        if (it != m_cache.end()) {
            return &it->second;
        }

        // Load new texture from disk via stb_image
        CachedTexture tex;
        int w = 0, h = 0, comp = 0;
        unsigned char* pixels = stbi_load(resolved.c_str(), &w, &h, &comp, 4);
        if (pixels && w > 0 && h > 0) {
            tex.channels = 4;
            tex.valid = true;
            tex.width = w;
            tex.height = h;
            tex.data.assign(pixels, pixels + (w * h * 4));

            // Fast compute averageColor using 32x32 point stride (max 1024 samples)
            int stepX = std::max(1, w / 32);
            int stepY = std::max(1, h / 32);
            double sumR = 0, sumG = 0, sumB = 0;
            int sampleCount = 0;
            for (int y = 0; y < h; y += stepY) {
                for (int x = 0; x < w; x += stepX) {
                    int idx = (y * w + x) * 4;
                    sumR += pixels[idx + 0];
                    sumG += pixels[idx + 1];
                    sumB += pixels[idx + 2];
                    sampleCount++;
                }
            }
            if (sampleCount > 0) {
                tex.averageColor = glm::vec3(sumR / sampleCount / 255.0, sumG / sampleCount / 255.0, sumB / sampleCount / 255.0);
            }
            stbi_image_free(pixels);
        }

        m_cache[resolved] = std::move(tex);
        return &m_cache[resolved];
    }
};
