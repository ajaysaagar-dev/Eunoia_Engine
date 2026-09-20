#pragma once
#include <string>
#include <filesystem>
#include <algorithm>

namespace AssetPath {

inline std::string Normalize(const std::string& path) {
    if (path.empty()) return "/Game";
    std::string norm = path;
    std::replace(norm.begin(), norm.end(), '\\', '/');

    // Strip duplicate slashes
    std::string clean;
    clean.reserve(norm.size());
    bool lastWasSlash = false;
    for (char c : norm) {
        if (c == '/') {
            if (!lastWasSlash) clean.push_back(c);
            lastWasSlash = true;
        } else {
            clean.push_back(c);
            lastWasSlash = false;
        }
    }

    // Ensure begins with /Game
    if (clean.rfind("/Game", 0) != 0) {
        if (!clean.empty() && clean[0] == '/') {
            clean = "/Game" + clean;
        } else {
            clean = "/Game/" + clean;
        }
    }

    // Remove trailing slash if not root
    if (clean.length() > 5 && clean.back() == '/') {
        clean.pop_back();
    }
    return clean;
}

inline std::string FromDiskPath(const std::filesystem::path& projectRoot, const std::filesystem::path& diskPath) {
    std::error_code ec;
    std::filesystem::path rel = std::filesystem::relative(diskPath, projectRoot, ec);
    std::string relStr = ec ? diskPath.filename().string() : rel.generic_string();

    // Strip extension for object name
    std::string stem = diskPath.stem().string();
    std::filesystem::path parentDir = rel.parent_path();

    std::string virtDir = "/Game";
    if (!ec && !parentDir.empty() && parentDir.generic_string() != ".") {
        virtDir += "/" + parentDir.generic_string();
    }
    return Normalize(virtDir + "/" + stem);
}

inline std::string GetDirectory(const std::string& virtualPath) {
    std::string norm = Normalize(virtualPath);
    auto pos = norm.find_last_of('/');
    if (pos == std::string::npos || pos == 0) return "/Game";
    return norm.substr(0, pos);
}

inline std::string GetObjectName(const std::string& virtualPath) {
    std::string norm = Normalize(virtualPath);
    auto pos = norm.find_last_of('/');
    if (pos == std::string::npos) return norm;
    return norm.substr(pos + 1);
}

inline std::string Combine(const std::string& virtualDir, const std::string& child) {
    std::string d = Normalize(virtualDir);
    std::string c = child;
    while (!c.empty() && c.front() == '/') c.erase(0, 1);
    return Normalize(d + "/" + c);
}

inline bool IsSubdirectory(const std::string& parentDir, const std::string& candidateChild) {
    std::string p = Normalize(parentDir);
    std::string c = Normalize(candidateChild);
    if (p == c) return true;
    if (p == "/Game") return true;
    return (c.rfind(p + "/", 0) == 0);
}

} // namespace AssetPath
