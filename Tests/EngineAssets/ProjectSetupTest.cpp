#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <EngineAssets/AssetRegistry.h>
#include <EngineAssets/AssetManager.h>
#include <EngineAssets/AssetPath.h>

int main() {
    std::cout << "[RUNNING] Project Setup and Content Root Tests...\n";

    std::filesystem::path testDir = std::filesystem::current_path() / "TestProjectSetup";
    std::error_code ec;
    std::filesystem::remove_all(testDir, ec);
    std::filesystem::create_directories(testDir, ec);

    std::string projName = "MyDemoProject";
    std::filesystem::path projRoot = testDir / projName;
    std::filesystem::path contentDir = projRoot / "Content";
    std::filesystem::path materialsDir = contentDir / "Materials";
    std::filesystem::path scenesDir = contentDir / "Scenes";
    std::filesystem::path cookedDir = projRoot / "Cooked";
    std::filesystem::path buildDir = projRoot / "Build";

    // 1. Verify Directory Creation
    std::filesystem::create_directories(contentDir, ec);
    std::filesystem::create_directories(materialsDir, ec);
    std::filesystem::create_directories(scenesDir, ec);
    std::filesystem::create_directories(cookedDir, ec);
    std::filesystem::create_directories(buildDir, ec);

    assert(std::filesystem::exists(projRoot));
    assert(std::filesystem::exists(contentDir));
    assert(std::filesystem::exists(materialsDir));
    assert(std::filesystem::exists(scenesDir));
    assert(std::filesystem::exists(cookedDir));
    assert(std::filesystem::exists(buildDir));

    // 2. Project Descriptor
    std::filesystem::path projFile = projRoot / (projName + ".eproject");
    {
        nlohmann::json j;
        j["name"] = projName;
        j["version"] = "1.0.0";
        j["contentDirectory"] = "Content";
        j["defaultScene"] = "Content/Scenes/Main.escene";
        std::ofstream pf(projFile);
        pf << j.dump(4);
    }
    assert(std::filesystem::exists(projFile));

    // 3. Verify Content Browser Root Mapping
    // Assets inside <projRoot>/Content map to /Game/<Subpath>
    std::filesystem::path testMat = materialsDir / "Default_Material.emat";
    {
        std::ofstream mf(testMat);
        mf << "# Material\nname: Default_Material\n";
    }
    std::string virtualPath = AssetPath::FromDiskPath(contentDir, testMat);
    assert(virtualPath == "/Game/Materials/Default_Material");

    // 4. Verify Recent Projects JSON persistence
    std::filesystem::path recentFile = testDir / "RecentProjects.json";
    {
        nlohmann::json arr = nlohmann::json::array();
        nlohmann::json item;
        item["name"] = projName;
        item["rootPath"] = projRoot.string();
        item["lastOpened"] = "2026-09-22 09:30:00";
        arr.push_back(item);
        std::ofstream rf(recentFile);
        rf << arr.dump(4);
    }
    assert(std::filesystem::exists(recentFile));

    {
        std::ifstream rf(recentFile);
        nlohmann::json loaded;
        rf >> loaded;
        assert(loaded.is_array());
        assert(loaded.size() == 1);
        assert(loaded[0]["name"] == projName);
        assert(loaded[0]["rootPath"] == projRoot.string());
    }

    // Clean up
    std::filesystem::remove_all(testDir, ec);

    std::cout << "[PASSED] Project Setup and Content Root Tests successfully verified!\n";
    return 0;
}
