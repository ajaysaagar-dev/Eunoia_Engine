#include "EngineUI.h"
#include "UndoManager.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>
#include <windows.h>
#include <commdlg.h>
#ifdef GetClassName
#undef GetClassName
#endif
#include <map>
#include "TextureManager.h"
#include "MeshImporter.h"
#include "EngineLogger.h"
#include "BehaviourRegistry.h"
#include "InputSystem.h"
#include <shellapi.h>

inline bool HasSceneStateChanged(const Scene& a, const Scene& b) {
    if (a.objects.size() != b.objects.size()) return true;
    if (a.pointLights.size() != b.pointLights.size()) return true;
    if (a.lightDirection != b.lightDirection || a.lightColor != b.lightColor || a.lightIntensity != b.lightIntensity) return true;
    if (a.ambientIntensity != b.ambientIntensity || a.enableShadows != b.enableShadows || a.shadowStrength != b.shadowStrength || a.shadowBias != b.shadowBias) return true;
    for (size_t i = 0; i < a.objects.size(); ++i) {
        const auto& o1 = a.objects[i];
        const auto& o2 = b.objects[i];
        if (o1.id != o2.id || o1.position != o2.position || o1.rotation != o2.rotation || o1.scale != o2.scale) return true;
        if (o1.color != o2.color || o1.metallic != o2.metallic || o1.roughness != o2.roughness) return true;
        if (o1.light.intensity != o2.light.intensity || o1.light.color != o2.light.color || o1.light.range != o2.light.range || o1.light.castShadows != o2.light.castShadows) return true;
        if (o1.materialName != o2.materialName || o1.name != o2.name) return true;
        if (o1.behaviours.size() != o2.behaviours.size()) return true;
    }
    for (size_t i = 0; i < a.pointLights.size(); ++i) {
        const auto& p1 = a.pointLights[i];
        const auto& p2 = b.pointLights[i];
        if (p1.position != p2.position || p1.color != p2.color || p1.intensity != p2.intensity || p1.range != p2.range || p1.castShadows != p2.castShadows || p1.enabled != p2.enabled) return true;
    }
    return false;
}

static EngineUI* g_pEngineUI = nullptr;

void AddEngineLog(const std::string& category, const std::string& message, int level) {
    if (g_pEngineUI) {
        g_pEngineUI->AddLog(category, message, level);
    }
}

EngineUI::EngineUI() {
    g_pEngineUI = this;
    AddLog("LogInit", "Eunoia-Editor Initialized (DirectX 12)", 2);
    AddLog("LogD3D12", "Hardware Adapter: NVIDIA GeForce RTX 3060 Laptop GPU (Feature Level 12_1)", 0);
    AddLog("LogWorld", "Default Level loaded with 5 initial Actors", 0);

    // Set C:\Projects\Eunoia-Engine\projects\test as root of project for Content Browser
    std::filesystem::path targetRoot = "C:\\Projects\\Eunoia-Engine\\projects\\test";
    std::error_code ec;
    if (!std::filesystem::exists(targetRoot, ec)) {
        std::filesystem::create_directories(targetRoot, ec);
    }
    contentRootPath = targetRoot;
    currentContentPath = targetRoot;
    TextureManager::Get().SetProjectRoot(contentRootPath);

    // Ensure Materials folder exists in project
    std::filesystem::path materialsDir = contentRootPath / "Materials";
    if (!std::filesystem::exists(materialsDir, ec)) {
        std::filesystem::create_directories(materialsDir, ec);
    }

    // Ensure Default_Material.emat exists in gray color
    std::filesystem::path defaultMatPath = materialsDir / "Default_Material.emat";
    if (!std::filesystem::exists(defaultMatPath, ec)) {
        MaterialAsset defaultMat;
        defaultMat.name = "Default_Material";
        defaultMat.filePath = defaultMatPath.string();
        defaultMat.baseColor = glm::vec3(0.55f, 0.55f, 0.55f);
        defaultMat.metallic = 0.0f;
        defaultMat.roughness = 0.5f;
        defaultMat.specular = 0.5f;
        defaultMat.assetId = AssetID::CreateRandom();
        defaultMat.virtualPath = "/Game/Materials/Default_Material";
        SaveMaterialFile(defaultMatPath.string(), defaultMat);
    }

    // Initialize Asset System (dev.md Section 7, 9)
    AssetManager::Get().Initialize(contentRootPath);
    AddLog("LogAsset", "Asset Registry initialized: " + std::to_string(AssetRegistry::Get().GetAssetCount()) + " assets registered with stable AssetIDs", 2);
}

void EngineUI::OpenReferenceViewer(const AssetID& id) {
    showReferenceViewer = true;
    if (id.IsValid()) {
        refViewerSelectedAsset = id;
    } else if (!AssetRegistry::Get().GetAllAssets().empty()) {
        refViewerSelectedAsset = AssetRegistry::Get().GetAllAssets().begin()->first;
    }
}

void EngineUI::OpenCookModal() {
    showCookModal = true;
    cookLog = "Ready to cook assets. Click 'Start Cooking' to package project assets into Cooked/Content/\n";
}

void EngineUI::AddLog(const std::string& category, const std::string& message, int level) {
    logs.push_back({ category, message, level });
    if (logs.size() > 250) {
        logs.erase(logs.begin(), logs.begin() + 50);
    }
}

void EngineUI::ToggleGameView(Scene& scene) {
    isGameView = !isGameView;
    if (isGameView) {
        prevShowGrid = scene.showGrid;
        prevShowGizmo = showGizmo;
        scene.showGrid = false;
        showGizmo = false;
        AddLog("LogViewport", "Game View Enabled [G]", 0);
    } else {
        scene.showGrid = prevShowGrid;
        showGizmo = prevShowGizmo;
        AddLog("LogViewport", "Game View Disabled [G]", 0);
    }
}

void EngineUI::ToggleImmersiveMode() {
    isImmersiveMode = !isImmersiveMode;
    if (isImmersiveMode) {
        prevShowOutliner = showOutliner;
        prevShowDetails = showDetails;
        prevShowBottomDrawer = showBottomDrawer;
        showOutliner = false;
        showDetails = false;
        showBottomDrawer = false;
        AddLog("LogViewport", "Immersive Viewport Mode Enabled [F11]", 0);
    } else {
        showOutliner = prevShowOutliner;
        showDetails = prevShowDetails;
        showBottomDrawer = prevShowBottomDrawer;
        AddLog("LogViewport", "Immersive Viewport Mode Restored [F11]", 0);
    }
}

bool EngineUI::SaveLevel(Scene& level) {
    if (currentLevelFilePath.empty()) {
        return SaveLevelAs(level);
    }
    if (SceneSerializer::SaveScene(level, currentLevelFilePath)) {
        levelUnsaved = false;
        AddLog("LogWorld", "Saved Level to: " + currentLevelFilePath, 2);
        return true;
    } else {
        AddLog("LogWorld", "Failed to save Level to: " + currentLevelFilePath, 3);
        return false;
    }
}

bool EngineUI::SaveLevelAs(Scene& level) {
    std::string path = ShowSaveFileDialog();
    if (path.empty()) return false;
    if (path.find(".elevel") == std::string::npos && path.find(".escene") == std::string::npos && path.find(".level") == std::string::npos) {
        path += ".elevel";
    }
    currentLevelFilePath = path;
    return SaveLevel(level);
}

bool EngineUI::OpenLevel(Scene& level) {
    std::string path = ShowOpenFileDialog();
    if (path.empty()) return false;
    return OpenLevelFromPath(level, path);
}

bool EngineUI::OpenLevelFromPath(Scene& level, const std::string& filePath) {
    if (SceneSerializer::LoadScene(level, filePath)) {
        currentLevelFilePath = filePath;
        levelUnsaved = false;
        AddLog("LogWorld", "Opened Level from: " + currentLevelFilePath + " (" + std::to_string(level.objects.size()) + " actors)", 2);
        return true;
    } else {
        AddLog("LogWorld", "Failed to open Level: " + filePath, 3);
        return false;
    }
}

std::string EngineUI::ShowSaveFileDialog() {
    OPENFILENAMEA ofn;
    char szFile[260] = {0};
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Eunoia Level (*.elevel;*.escene;*.level)\0*.elevel;*.escene;*.level\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = "C:\\Projects\\Eunoia-Engine\\projects\\test";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    if (GetSaveFileNameA(&ofn) == TRUE) {
        return std::string(ofn.lpstrFile);
    }
    return "";
}

std::string EngineUI::ShowOpenFileDialog() {
    OPENFILENAMEA ofn;
    char szFile[260] = {0};
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Eunoia Level (*.elevel;*.escene;*.level)\0*.elevel;*.escene;*.level\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = "C:\\Projects\\Eunoia-Engine\\projects\\test";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&ofn) == TRUE) {
        return std::string(ofn.lpstrFile);
    }
    return "";
}

std::string EngineUI::ShowOpenMeshDialog() {
    OPENFILENAMEA ofn;
    char szFile[260] = {0};
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "3D Models (*.obj;*.gltf;*.glb;*.fbx)\0*.obj;*.gltf;*.glb;*.fbx\0FBX Model (*.fbx)\0*.fbx\0Wavefront OBJ (*.obj)\0*.obj\0glTF Model (*.gltf;*.glb)\0*.gltf;*.glb\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = "C:\\Projects\\Eunoia-Engine\\projects\\test";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&ofn) == TRUE) {
        return std::string(ofn.lpstrFile);
    }
    return "";
}

std::string EngineUI::ShowSaveMeshLocationDialog(const std::string& defaultFilename, const std::string& initialDir) {
    OPENFILENAMEA ofn;
    char szFile[260] = {0};
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    if (!defaultFilename.empty()) {
        strncpy(szFile, defaultFilename.c_str(), sizeof(szFile) - 1);
    }
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "3D Models (*.obj;*.gltf;*.glb;*.fbx)\0*.obj;*.gltf;*.glb;*.fbx\0FBX Model (*.fbx)\0*.fbx\0Wavefront OBJ (*.obj)\0*.obj\0glTF Model (*.gltf;*.glb)\0*.gltf;*.glb\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = "Select Save Location in Project Folder";
    ofn.lpstrInitialDir = initialDir.c_str();
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    if (GetSaveFileNameA(&ofn) == TRUE) {
        return std::string(ofn.lpstrFile);
    }
    return "";
}

void EngineUI::ImportMeshWithSavePrompt(Scene& level) {
    std::string sourceMesh = ShowOpenMeshDialog();
    if (sourceMesh.empty()) return;

    std::filesystem::path srcPath(sourceMesh);
    std::string filename = srcPath.filename().string();

    // Default target directory inside project
    std::string diskSub = currentVirtualDir;
    if (diskSub.rfind("/Game", 0) == 0) diskSub = diskSub.substr(5);
    if (!diskSub.empty() && diskSub[0] == '/') diskSub.erase(0, 1);
    std::filesystem::path defaultDir = contentRootPath / diskSub;
    std::error_code ec;
    if (defaultDir == contentRootPath) {
        std::filesystem::path modelsDir = contentRootPath / "Models";
        if (std::filesystem::exists(modelsDir, ec)) {
            defaultDir = modelsDir;
        }
    }

    std::string saveLoc = ShowSaveMeshLocationDialog(filename, defaultDir.string());
    if (saveLoc.empty()) {
        AddLog("LogMesh", "Mesh import cancelled by user.", 1);
        return;
    }

    std::filesystem::path destPath(saveLoc);
    std::filesystem::create_directories(destPath.parent_path(), ec);

    // Copy file into project location
    if (std::filesystem::absolute(srcPath) != std::filesystem::absolute(destPath)) {
        std::filesystem::copy_file(srcPath, destPath, std::filesystem::copy_options::overwrite_existing, ec);
        // Also copy companion .mtl if it exists
        std::string ext = srcPath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".obj") {
            std::filesystem::path srcMtl = srcPath.parent_path() / (srcPath.stem().string() + ".mtl");
            if (std::filesystem::exists(srcMtl, ec)) {
                std::filesystem::path destMtl = destPath.parent_path() / (destPath.stem().string() + ".mtl");
                std::filesystem::copy_file(srcMtl, destMtl, std::filesystem::copy_options::overwrite_existing, ec);
            }
        }
    }

    // Sync Asset Registry so Content Browser immediately has the new asset
    AssetRegistry::Get().ScanAndSync(contentRootPath);

    // Spawn mesh in level
    GameObject& newObj = level.AddImportedMesh(destPath.string(), {0.0f, 0.5f, 0.0f});
    level.selectedId = newObj.id;
    AddLog("LogMesh", "Imported and saved 3D Mesh to project: " + destPath.string() + ", spawned in Level", 2);
}

void EngineUI::HandleFileDrop(const char** paths, int count) {
    if (!paths || count <= 0) return;

    std::string diskSub = currentVirtualDir;
    if (diskSub.rfind("/Game", 0) == 0) diskSub = diskSub.substr(5);
    if (!diskSub.empty() && diskSub[0] == '/') diskSub.erase(0, 1);
    std::filesystem::path targetDir = contentRootPath / diskSub;
    std::error_code ec;
    std::filesystem::create_directories(targetDir, ec);

    int importedCount = 0;
    for (int i = 0; i < count; ++i) {
        if (!paths[i]) continue;
        std::filesystem::path srcPath(paths[i]);
        if (!std::filesystem::exists(srcPath, ec)) continue;

        std::string ext = srcPath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        bool isModel = (ext == ".obj" || ext == ".gltf" || ext == ".glb" || ext == ".fbx");
        bool isTex = (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga");
        bool isMat = (ext == ".emat" || ext == ".mat");
        bool isLevel = (ext == ".elevel" || ext == ".level" || ext == ".escene" || ext == ".scene");

        if (isModel || isTex || isMat || isLevel) {
            std::filesystem::path destPath = targetDir / srcPath.filename();
            if (std::filesystem::absolute(srcPath) != std::filesystem::absolute(destPath)) {
                std::filesystem::copy_file(srcPath, destPath, std::filesystem::copy_options::overwrite_existing, ec);
                if (ext == ".obj") {
                    std::filesystem::path srcMtl = srcPath.parent_path() / (srcPath.stem().string() + ".mtl");
                    if (std::filesystem::exists(srcMtl, ec)) {
                        std::filesystem::path destMtl = targetDir / (srcPath.stem().string() + ".mtl");
                        std::filesystem::copy_file(srcMtl, destMtl, std::filesystem::copy_options::overwrite_existing, ec);
                    }
                }
            }
            importedCount++;
            AddLog("LogContent", "Imported from system: " + srcPath.filename().string() + " into " + currentVirtualDir, 2);
        }
    }

    if (importedCount > 0) {
        AssetRegistry::Get().ScanAndSync(contentRootPath);
        AddLog("LogContent", "Successfully imported " + std::to_string(importedCount) + " model/asset file(s) into Content Browser.", 2);
    }
}

glm::vec3 EngineUI::CalculateDropSpawnPosition(OrbitCamera& camera, float mouseX, float mouseY, float screenW, float screenH) {
    if (screenW <= 0.0f || screenH <= 0.0f) return camera.target;
    float ndcX = (2.0f * mouseX) / screenW - 1.0f;
    float ndcY = 1.0f - (2.0f * mouseY) / screenH;
    float aspect = screenW / screenH;

    glm::mat4 proj = camera.GetProjectionMatrix(aspect);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 invVP = glm::inverse(proj * view);

    glm::vec4 pNear = invVP * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
    glm::vec4 pFar  = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
    if (pNear.w != 0.0f) pNear /= pNear.w;
    if (pFar.w != 0.0f)  pFar  /= pFar.w;

    glm::vec3 rayOrig = glm::vec3(pNear);
    glm::vec3 rayDir  = glm::normalize(glm::vec3(pFar - pNear));

    glm::vec3 spawnPos = camera.target;
    if (std::abs(rayDir.y) > 0.0001f) {
        float t = -rayOrig.y / rayDir.y;
        if (t > 0.0f && t < 200.0f) {
            spawnPos = rayOrig + rayDir * t;
            spawnPos.y = 0.5f;
            return spawnPos;
        }
    }
    spawnPos.y = 0.5f;
    return spawnPos;
}

void EngineUI::RenderViewportDropTarget(Scene& level, OrbitCamera& camera) {
    const ImGuiPayload* curPayload = ImGui::GetDragDropPayload();
    if (!curPayload || !curPayload->IsDataType("ASSET_ID")) return;

    ImGuiViewport* vp = ImGui::GetMainViewport();
    auto vpRect = GetViewportRect(vp->Size.x, vp->Size.y);
    float vpX = vp->Pos.x + vpRect.x;
    float vpY = vp->Pos.y + vpRect.y;
    float vpW = vpRect.width;
    float vpH = vpRect.height;

    if (vpW <= 10.0f || vpH <= 10.0f) return;

    ImGui::SetNextWindowPos(ImVec2(vpX, vpY));
    ImGui::SetNextWindowSize(ImVec2(vpW, vpH));
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;

    if (ImGui::Begin("##ViewportDropArea", nullptr, flags)) {
        ImGui::InvisibleButton("##ViewportDropBtn", ImVec2(vpW, vpH));

        if (ImGui::BeginDragDropTarget()) {
            ImDrawList* dl = ImGui::GetForegroundDrawList();
            dl->AddRect(ImVec2(vpX + 4.0f, vpY + 4.0f), ImVec2(vpX + vpW - 4.0f, vpY + vpH - 4.0f),
                        IM_COL32(50, 180, 255, 180), 0.0f, 0, 2.5f);

            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_ID")) {
                std::string idStr = (const char*)payload->Data;
                AssetID id = AssetID::FromString(idStr);
                const AssetMetadata* meta = AssetRegistry::Get().GetMetadata(id);
                if (meta) {
                    if (meta->type == AssetType::Mesh) {
                        std::filesystem::path fullMesh = contentRootPath / meta->sourcePath;
                        ImVec2 mousePos = ImGui::GetMousePos();
                        glm::vec3 spawnPos = CalculateDropSpawnPosition(camera, mousePos.x - vpX, mousePos.y - vpY, vpW, vpH);

                        GameObject& newObj = level.AddImportedMesh(fullMesh.string(), spawnPos);
                        level.selectedId = newObj.id;
                        AddLog("LogMesh", "Spawned 3D Object '" + meta->objectName + "' into Level at cursor (" +
                               std::to_string(spawnPos.x).substr(0, 4) + ", " +
                               std::to_string(spawnPos.z).substr(0, 4) + ")", 2);
                    } else if (meta->type == AssetType::Level || meta->type == AssetType::Scene) {
                        std::filesystem::path fullLevel = contentRootPath / meta->sourcePath;
                        OpenLevelFromPath(level, fullLevel.string());
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::End();
    }
}

void EngineUI::SetupTheme() {
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigDebugHighlightIdConflicts = false;

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Friendly, comfortable UI sizing and clean modern styling
    style.WindowRounding    = 0.0f;
    style.ChildRounding     = 3.0f;
    style.FrameRounding     = 4.0f;
    style.PopupRounding     = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding      = 4.0f;
    style.TabRounding       = 4.0f;

    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.PopupBorderSize   = 1.0f;

    style.WindowPadding     = ImVec2(12.0f, 10.0f);
    style.FramePadding      = ImVec2(8.0f, 6.0f);
    style.ItemSpacing       = ImVec2(9.0f, 7.0f);
    style.ItemInnerSpacing  = ImVec2(7.0f, 5.0f);
    style.ScrollbarSize     = 16.0f;
    style.GrabMinSize       = 14.0f;

    // Color Palette: #2B2B2B is RGB (0.169f, 0.169f, 0.169f)
    colors[ImGuiCol_Text]                  = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.55f, 0.57f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.169f, 0.169f, 0.169f, 0.98f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.125f, 0.125f, 0.125f, 0.70f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.155f, 0.155f, 0.155f, 0.98f);
    colors[ImGuiCol_Border]                = ImVec4(0.08f, 0.08f, 0.08f, 0.85f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.120f, 0.120f, 0.120f, 0.90f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.200f, 0.200f, 0.200f, 1.00f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.240f, 0.240f, 0.240f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.145f, 0.145f, 0.145f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.160f, 0.160f, 0.160f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.130f, 0.130f, 0.130f, 0.80f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.169f, 0.169f, 0.169f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.125f, 0.125f, 0.125f, 0.50f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.260f, 0.260f, 0.260f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.350f, 0.350f, 0.350f, 0.90f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.420f, 0.420f, 0.420f, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.12f, 0.65f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.12f, 0.60f, 0.95f, 0.90f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.20f, 0.70f, 1.00f, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(0.140f, 0.140f, 0.140f, 0.90f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.220f, 0.220f, 0.220f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.080f, 0.500f, 0.900f, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(0.140f, 0.140f, 0.140f, 0.80f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.200f, 0.200f, 0.200f, 0.90f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.080f, 0.500f, 0.900f, 1.00f);
    colors[ImGuiCol_Separator]             = ImVec4(0.120f, 0.120f, 0.120f, 0.80f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.12f, 0.60f, 0.95f, 0.80f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.12f, 0.65f, 1.00f, 1.00f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.135f, 0.135f, 0.135f, 0.85f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.210f, 0.210f, 0.210f, 0.95f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.080f, 0.500f, 0.900f, 1.00f);
}

void EngineUI::Render(Scene& scene, OrbitCamera& camera, float fps, float frameTimeMs, uint32_t vertexCount, uint32_t indexCount, bool& outShouldExit) {
    if (!scene.onPreChange) {
        scene.onPreChange = [&scene](const std::string& action) {
            UndoManager::Get().RecordSnapshot(scene, action);
        };
    }

    ImGuizmo::BeginFrame();

    // Unreal Keyboard Shortcuts (when not typing and not currently in free fly mode)
    if (!ImGui::GetIO().WantTextInput && !camera.isFlying) {
        // F: Focus selected actor
        if (ImGui::IsKeyPressed(ImGuiKey_F) && scene.selectedId != -1) {
            GameObject* obj = scene.GetSelected();
            if (obj) {
                camera.FocusOn(obj->position, obj->scale);
                AddLog("LogViewport", "Focused Viewport on Actor: " + obj->name + " [F]", 0);
            }
        }

        // G: Toggle Game View (Hide Grid & Gizmos)
        if (ImGui::IsKeyPressed(ImGuiKey_G)) {
            ToggleGameView(scene);
        }

        // F11: Toggle Immersive Viewport
        if (ImGui::IsKeyPressed(ImGuiKey_F11)) {
            ToggleImmersiveMode();
        }

        // Transform Tool shortcuts
        if (ImGui::IsKeyPressed(ImGuiKey_W)) {
            currentGizmoOperation = ImGuizmo::TRANSLATE;
            AddLog("LogEditor", "Active Tool: Translate (W)", 0);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_E)) {
            currentGizmoOperation = ImGuizmo::ROTATE;
            AddLog("LogEditor", "Active Tool: Rotate (E)", 0);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_R)) {
            currentGizmoOperation = ImGuizmo::SCALE;
            AddLog("LogEditor", "Active Tool: Scale (R)", 0);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Q)) {
            currentGizmoMode = (currentGizmoMode == ImGuizmo::WORLD) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
            AddLog("LogEditor", (currentGizmoMode == ImGuizmo::WORLD) ? "Coordinate Space: World" : "Coordinate Space: Local", 0);
        }
        // Undo / Redo Shortcuts (52 steps)
        if (ImGui::GetIO().KeyCtrl && !ImGui::GetIO().KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z)) {
            if (UndoManager::Get().CanUndo()) {
                std::string act = UndoManager::Get().GetUndoActionName();
                UndoManager::Get().Undo(scene);
                AddLog("LogEditor", "Undo: " + act, 0);
            }
        }
        if (ImGui::GetIO().KeyCtrl && (ImGui::IsKeyPressed(ImGuiKey_Y) || (ImGui::GetIO().KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z)))) {
            if (UndoManager::Get().CanRedo()) {
                std::string act = UndoManager::Get().GetRedoActionName();
                UndoManager::Get().Redo(scene);
                AddLog("LogEditor", "Redo: " + act, 0);
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Z) && !ImGui::GetIO().KeyCtrl) {
            gizmoUseCenter = !gizmoUseCenter;
            AddLog("LogEditor", gizmoUseCenter ? "Gizmo Position: Center (Z)" : "Gizmo Position: Pivot (Z)", 0);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) && scene.selectedId != -1) {
            GameObject* o = scene.GetSelected();
            if (o) {
                UndoManager::Get().RecordSnapshot(scene, "Delete Actor: " + o->name);
                AddLog("LogActor", "Deleted Actor: " + o->name, 1);
            }
            scene.RemoveObject(scene.selectedId);
        }
        if (ImGui::GetIO().KeyCtrl && (ImGui::IsKeyPressed(ImGuiKey_D) || ImGui::IsKeyPressed(ImGuiKey_W)) && scene.selectedId != -1) {
            if (!ImGui::GetIO().WantTextInput) {
                UndoManager::Get().RecordSnapshot(scene, "Duplicate Actor");
                GameObject* copy = scene.DuplicateObject(scene.selectedId);
                if (copy) {
                    std::string logMsg = "Duplicated Actor: " + copy->name;
                    if (!copy->childIds.empty()) {
                        logMsg += " (along with " + std::to_string(copy->childIds.size()) + " children)";
                    }
                    AddLog("LogActor", logMsg, 2);
                    EngineLogger::Get().LogAction("DUPLICATE_ACTOR", copy->name, "Children: " + std::to_string(copy->childIds.size()));
                }
            }
        }
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
            if (ImGui::GetIO().KeyShift) SaveSceneAs(scene);
            else SaveScene(scene);
        }
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O)) {
            OpenScene(scene);
        }
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N)) {
            UndoManager::Get().RecordSnapshot(scene, "Clear Scene");
            scene.Clear();
            currentLevelFilePath = "";
            AddLog("LogWorld", "Cleared Level (New Level)", 0);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            scene.selectedId = -1;
        }
    }

    // Play Mode Presentation & Stop-on-Delete (dev.md Section 32, 34)
    if (scene.isPlayMode) {
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) || InputSystem::Get().IsKeyPressed(Key::Delete)) {
            ExitPlayMode(scene);
            return;
        }

        ImGuiViewport* mainVp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(mainVp->WorkPos.x + 16.0f, mainVp->WorkPos.y + 16.0f), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.75f);
        ImGuiWindowFlags playFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
        if (ImGui::Begin("##PlayModeBanner", nullptr, playFlags)) {
            ImGui::TextColored(ImVec4(0.2f, 0.95f, 0.4f, 1.0f), "▶ GAME VIEW (PLAY MODE)");
            ImGui::SameLine();
            ImGui::TextDisabled("| Press [DELETE] to Stop");
            ImGui::SameLine();
            if (ImGui::Button("⏹ Stop (DELETE)")) {
                ExitPlayMode(scene);
            }
        }
        ImGui::End();

        // Game View occupies the available area; editor UI is hidden
        return;
    }

    // Global editor change tracking for continuous edits (sliders, drag inputs, colors, text fields)
    static bool s_hasPreEditSnapshot = false;
    static Scene s_preEditSceneSnapshot;
    if (ImGui::IsAnyItemActive()) {
        if (!s_hasPreEditSnapshot) {
            s_hasPreEditSnapshot = true;
            s_preEditSceneSnapshot = scene;
        }
    } else if (s_hasPreEditSnapshot) {
        s_hasPreEditSnapshot = false;
        if (HasSceneStateChanged(s_preEditSceneSnapshot, scene)) {
            UndoManager::Get().RecordSnapshot(s_preEditSceneSnapshot, "Modify Properties");
        }
    }

    // 1. Top Menu Bar (Matches UI_Ref.svg "Menu Bar" top panel)
    RenderTopMenuBar(scene, camera, outShouldExit);

    // 2. Outliner (Matches UI_Ref.svg "Outliner" left sidebar)
    if (showOutliner) {
        RenderOutliner(scene);
    }

    // 3. Details Panel (Matches UI_Ref.svg "Details" right sidebar)
    if (showDetails) {
        RenderDetails(scene);
    }

    // 4. Content Browser (Matches UI_Ref.svg "Content Browser" full-width bottom panel)
    if (showBottomDrawer) {
        RenderContentBrowser(scene);
    }

    // 5. Central 3D Viewport Overlay & Drop Target
    if (showViewportOverlay) {
        RenderViewportOverlay(scene, camera, fps, frameTimeMs);
    }
    RenderViewportDropTarget(scene, camera);

    if (showHelpModal) {
        RenderHelpModal();
    }

    // 6. Material Editor Window (Opened via double-click on material in Content Browser)
    if (showMaterialEditor) {
        RenderMaterialEditor(scene);
    }

    // 7. Asset Reference Viewer (dev.md Section 33)
    if (showReferenceViewer) {
        RenderReferenceViewer(scene);
    }

    // 8. Asset Cooking Pipeline Modal (dev.md Section 24, 25)
    if (showCookModal) {
        RenderCookModal();
    }

    // 9. In-Engine Code Editor (dev.md & User Request)
    if (showCodeEditor) {
        RenderCodeEditor();
    }

    // Undo History Window (52 steps capacity)
    if (showUndoHistory) {
        ImGui::SetNextWindowSize(ImVec2(360, 440), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Undo History (52 Steps)###UndoHistoryWin", &showUndoHistory)) {
            ImGui::TextColored(ImVec4(0.12f, 0.68f, 1.00f, 1.0f), "History Stack: %zu / 52 steps", UndoManager::Get().GetUndoCount());
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear History")) {
                UndoManager::Get().Clear();
            }
            ImGui::Separator();

            const auto& stack = UndoManager::Get().GetUndoStack();
            if (stack.empty()) {
                ImGui::TextDisabled("No undo history yet. Edit or transform objects!");
            } else {
                for (int i = (int)stack.size() - 1; i >= 0; --i) {
                    std::string label = "#" + std::to_string(i + 1) + ": " + stack[i].actionName;
                    if (ImGui::Selectable(label.c_str(), false)) {
                        UndoManager::Get().JumpToUndoStep(i, scene);
                        AddLog("LogEditor", "Jumped to: " + stack[i].actionName, 0);
                    }
                }
            }
        }
        ImGui::End();
    }

    // 9. Interactive 3D Transform Gizmo
    ImGuiViewport* vp = ImGui::GetMainViewport();
    auto vpRect = GetViewportRect(vp->Size.x, vp->Size.y);
    RenderGizmo(scene, camera, vpRect.width, vpRect.height);
}

void EngineUI::RenderTopMenuBar(Scene& scene, OrbitCamera& camera, bool& outShouldExit) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    float topBarW = vp->Size.x;

    ImGui::SetNextWindowPos(ImVec2(vp->Pos.x, vp->Pos.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(topBarW, topBarHeight), ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 6.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    if (ImGui::Begin("Menu Bar", nullptr, flags)) {
        if (ImGui::BeginMenuBar()) {
            // Eunoia Brand Badge
            ImGui::TextColored(ImVec4(0.12f, 0.68f, 1.00f, 1.0f), "[Eunoia]");
            ImGui::SameLine();

            // Menu Bar Dropdowns
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Level", "Ctrl+N")) {
                    scene.Clear();
                    currentLevelFilePath = "";
                    AddLog("LogWorld", "Cleared Level (New Level)", 0);
                }
                if (ImGui::MenuItem("Open Level...", "Ctrl+O")) {
                    OpenLevel(scene);
                }
                if (ImGui::MenuItem("Save Level", "Ctrl+S")) {
                    SaveLevel(scene);
                }
                if (ImGui::MenuItem("Save Level As...", "Ctrl+Shift+S")) {
                    SaveLevelAs(scene);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Reset Default Level")) {
                    scene.LoadDefaultScene();
                    currentLevelFilePath = "";
                    AddLog("LogWorld", "Loaded Default Level", 0);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4")) {
                    outShouldExit = true;
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit")) {
                std::string undoLabel = "Undo";
                if (UndoManager::Get().CanUndo()) undoLabel += " " + UndoManager::Get().GetUndoActionName();
                if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, UndoManager::Get().CanUndo())) {
                    UndoManager::Get().Undo(scene);
                    AddLog("LogEditor", "Undo: " + undoLabel, 0);
                }

                std::string redoLabel = "Redo";
                if (UndoManager::Get().CanRedo()) redoLabel += " " + UndoManager::Get().GetRedoActionName();
                if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y", false, UndoManager::Get().CanRedo())) {
                    UndoManager::Get().Redo(scene);
                    AddLog("LogEditor", "Redo: " + redoLabel, 0);
                }
                ImGui::Separator();
                ImGui::MenuItem("📜 Undo History (52 Steps)", nullptr, &showUndoHistory);
                ImGui::Separator();

                if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, scene.selectedId != -1)) {
                    UndoManager::Get().RecordSnapshot(scene, "Duplicate Actor");
                    GameObject* copy = scene.DuplicateObject(scene.selectedId);
                    if (copy) {
                        std::string logMsg = "Duplicated Actor: " + copy->name;
                        if (!copy->childIds.empty()) {
                            logMsg += " (along with " + std::to_string(copy->childIds.size()) + " children)";
                        }
                        AddLog("LogActor", logMsg, 2);
                        EngineLogger::Get().LogAction("DUPLICATE_ACTOR", copy->name, "Children: " + std::to_string(copy->childIds.size()));
                    }
                }
                if (ImGui::MenuItem("Delete", "Del", false, scene.selectedId != -1)) {
                    GameObject* o = scene.GetSelected();
                    if (o) {
                        UndoManager::Get().RecordSnapshot(scene, "Delete Actor: " + o->name);
                        AddLog("LogActor", "Deleted Actor: " + o->name, 1);
                    }
                    scene.RemoveObject(scene.selectedId);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Deselect All", "Esc")) {
                    scene.selectedId = -1;
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Window")) {
                ImGui::MenuItem("Outliner (Left Sidebar)", nullptr, &showOutliner);
                ImGui::MenuItem("Details (Right Sidebar)", nullptr, &showDetails);
                ImGui::MenuItem("Content Browser (Bottom Dock)", nullptr, &showBottomDrawer);
                ImGui::MenuItem("Viewport Overlay", nullptr, &showViewportOverlay);
                ImGui::Separator();
                ImGui::MenuItem("📜 Undo History (52 Steps)", nullptr, &showUndoHistory);
                ImGui::MenuItem("🎨 Material Editor", nullptr, &showMaterialEditor);
                ImGui::MenuItem("🔍 Asset Reference Viewer", nullptr, &showReferenceViewer);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Assets")) {
                if (ImGui::MenuItem("🔄 Scan & Sync Asset Registry")) {
                    AssetRegistry::Get().ScanAndSync(contentRootPath);
                    AddLog("LogAsset", "Rescanned and synchronized Asset Registry (" + std::to_string(AssetRegistry::Get().GetAssetCount()) + " assets)", 2);
                }
                if (ImGui::MenuItem("🔍 Asset Reference Viewer")) {
                    OpenReferenceViewer();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("📦 Cook Project Assets (Packaging)")) {
                    OpenCookModal();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Actor")) {
                if (ImGui::MenuItem("Cube")) { scene.AddObject(PrimitiveType::Cube, {0.0f, 0.5f, 0.0f}, {0.85f, 0.35f, 0.25f}); AddLog("LogActor", "Spawned Cube", 2); }
                if (ImGui::MenuItem("Plane")) { scene.AddObject(PrimitiveType::Plane, {0.0f, 0.0f, 0.0f}, {0.35f, 0.65f, 0.45f}); AddLog("LogActor", "Spawned Plane", 2); }
                if (ImGui::MenuItem("Sphere")) { scene.AddObject(PrimitiveType::Sphere, {0.0f, 0.6f, 0.0f}, {0.25f, 0.65f, 0.95f}); AddLog("LogActor", "Spawned Sphere", 2); }
                if (ImGui::MenuItem("Cylinder")) { scene.AddObject(PrimitiveType::Cylinder, {0.0f, 0.6f, 0.0f}, {0.95f, 0.80f, 0.20f}); AddLog("LogActor", "Spawned Cylinder", 2); }
                if (ImGui::MenuItem("Pyramid / Cone")) { scene.AddObject(PrimitiveType::Pyramid, {0.0f, 0.0f, 0.0f}, {0.90f, 0.50f, 0.20f}); AddLog("LogActor", "Spawned Pyramid", 2); }
                if (ImGui::MenuItem("Torus")) { scene.AddObject(PrimitiveType::Torus, {0.0f, 0.6f, 0.0f}, {0.75f, 0.30f, 0.85f}); AddLog("LogActor", "Spawned Torus", 2); }
                ImGui::Separator();
                if (ImGui::MenuItem("🧊 Import 3D Mesh (OBJ/GLTF)...")) {
                    ImportMeshWithSavePrompt(scene);
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("Eunoia-Editor Guide")) {
                    showHelpModal = true;
                }
                ImGui::EndMenu();
            }

            ImGui::TextDisabled("|");

            // Quick Add Actor button
            if (ImGui::Button("+ Add Actor")) {
                ImGui::OpenPopup("QuickAddPopup");
            }
            if (ImGui::BeginPopup("QuickAddPopup")) {
                ImGui::TextDisabled("PRIMITIVES");
                if (ImGui::MenuItem("Cube")) { scene.AddObject(PrimitiveType::Cube); AddLog("LogActor", "Spawned Cube", 2); }
                if (ImGui::MenuItem("Plane")) { scene.AddObject(PrimitiveType::Plane); AddLog("LogActor", "Spawned Plane", 2); }
                if (ImGui::MenuItem("Sphere")) { scene.AddObject(PrimitiveType::Sphere); AddLog("LogActor", "Spawned Sphere", 2); }
                if (ImGui::MenuItem("Cylinder")) { scene.AddObject(PrimitiveType::Cylinder); AddLog("LogActor", "Spawned Cylinder", 2); }
                if (ImGui::MenuItem("Pyramid / Cone")) { scene.AddObject(PrimitiveType::Pyramid); AddLog("LogActor", "Spawned Pyramid", 2); }
                if (ImGui::MenuItem("Torus")) { scene.AddObject(PrimitiveType::Torus); AddLog("LogActor", "Spawned Torus", 2); }
                ImGui::Separator();
                ImGui::TextDisabled("CUSTOM MESH");
                if (ImGui::MenuItem("Import 3D Mesh (OBJ/GLTF)...")) {
                    ImportMeshWithSavePrompt(scene);
                }
                ImGui::EndPopup();
            }

            ImGui::TextDisabled("|");

            // Transform Gizmo Tools
            auto DrawToolBtn = [&](const char* label, int op, const char* tooltip) {
                bool active = (currentGizmoOperation == op);
                if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.50f, 0.90f, 1.0f));
                if (ImGui::Button(label)) currentGizmoOperation = op;
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);
                if (active) ImGui::PopStyleColor();
            };

            DrawToolBtn("Move (W)", ImGuizmo::TRANSLATE, "Translate Tool (W)");
            DrawToolBtn("Rotate (E)", ImGuizmo::ROTATE, "Rotate Tool (E)");
            DrawToolBtn("Scale (R)", ImGuizmo::SCALE, "Scale Tool (R)");

            const char* spaceStr = (currentGizmoMode == ImGuizmo::WORLD) ? "World (Q)" : "Local (Q)";
            if (ImGui::Button(spaceStr)) {
                currentGizmoMode = (currentGizmoMode == ImGuizmo::WORLD) ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle World / Local Coordinate Space (Q)");

            const char* pivotStr = gizmoUseCenter ? "Center (Z)" : "Pivot (Z)";
            if (ImGui::Button(pivotStr)) {
                gizmoUseCenter = !gizmoUseCenter;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Gizmo Position: Pivot Point / Geometric Center (Z)");

            ImGui::Checkbox("Snap", &useSnap);

            ImGui::TextDisabled("|");

            // PIE Controls (Play In Editor) (dev.md Section 30-36)
            ImGui::PushStyleColor(ImGuiCol_Button, scene.isPlayMode ? ImVec4(0.18f, 0.75f, 0.32f, 1.0f) : ImVec4(0.12f, 0.55f, 0.25f, 1.0f));
            if (ImGui::Button("▶ Play")) {
                EnterPlayMode(scene);
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enter Play Mode (Runs Behaviours & Simulation; Press DELETE to stop)");
            ImGui::PopStyleColor();

            if (scene.isPlayMode) {
                ImGui::SameLine();
                if (ImGui::Button("⏹ Stop (DELETE)")) {
                    ExitPlayMode(scene);
                }
            }

            if (ImGui::Button("Reset View")) {
                camera.SetPreset("Perspective");
            }

            ImGui::TextDisabled("|");
            if (ImGui::Button("🔍 Ref Viewer")) {
                OpenReferenceViewer();
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Open Asset Reference / Dependency Viewer (dev.md Section 33)");

            if (ImGui::Button("📦 Cook")) {
                OpenCookModal();
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Cook & Package Project Assets (dev.md Section 24, 25)");

            // Right side info
            float rightWidth = 250.0f;
            if (ImGui::GetContentRegionAvail().x > rightWidth) {
                ImGui::SameLine(ImGui::GetWindowWidth() - rightWidth);
                ImGui::TextColored(ImVec4(0.40f, 0.70f, 1.00f, 1.00f), "DirectX 12 | RTX 3060");
            }

            ImGui::EndMenuBar();
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
}

void EngineUI::RenderViewportOverlay(Scene& scene, OrbitCamera& camera, float fps, float frameTimeMs) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    auto vpRect = GetViewportRect(vp->Size.x, vp->Size.y);
    float overlayX = vp->Pos.x + vpRect.x + 8.0f;
    float overlayY = vp->Pos.y + vpRect.y + 8.0f;

    // Floating Viewport Toolbar (Top Left of 3D Viewport)
    ImGui::SetNextWindowPos(ImVec2(overlayX, overlayY), ImGuiCond_Always);
    ImGuiWindowFlags vpFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                               ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                               ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
    ImGui::SetNextWindowBgAlpha(0.75f);
    if (ImGui::Begin("##ViewportOverlay", nullptr, vpFlags)) {
        if (isGameView) {
            ImGui::TextColored(ImVec4(0.25f, 0.90f, 0.45f, 1.0f), "[GAME VIEW: G]");
            ImGui::SameLine();
        }
        if (isImmersiveMode) {
            ImGui::TextColored(ImVec4(1.00f, 0.70f, 0.20f, 1.0f), "[IMMERSIVE: F11]");
            ImGui::SameLine();
        }
        if (camera.isFlying) {
            ImGui::TextColored(ImVec4(0.20f, 0.75f, 1.00f, 1.0f), "[FLYING: WASDQE | Spd: %.1f]", camera.moveSpeed);
            ImGui::SameLine();
        }

        if (ImGui::Button("Perspective v")) {
            ImGui::OpenPopup("CamPresetPopup");
        }
        if (ImGui::BeginPopup("CamPresetPopup")) {
            if (ImGui::MenuItem("Perspective")) camera.SetPreset("Perspective");
            if (ImGui::MenuItem("Top")) camera.SetPreset("Top");
            if (ImGui::MenuItem("Front")) camera.SetPreset("Front");
            if (ImGui::MenuItem("Side")) camera.SetPreset("Side");
            if (ImGui::MenuItem("Isometric")) camera.SetPreset("Isometric");
            ImGui::EndPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Lit v")) {
            ImGui::OpenPopup("ViewModePopup");
        }
        if (ImGui::BeginPopup("ViewModePopup")) {
            if (ImGui::MenuItem("Lit", nullptr, viewMode == 0)) viewMode = 0;
            if (ImGui::MenuItem("Unlit", nullptr, viewMode == 1)) viewMode = 1;
            ImGui::EndPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Show v")) {
            ImGui::OpenPopup("ShowPopup");
        }
        if (ImGui::BeginPopup("ShowPopup")) {
            if (ImGui::MenuItem("Grid", nullptr, scene.showGrid)) scene.showGrid = !scene.showGrid;
            if (ImGui::MenuItem("Transform Gizmo", nullptr, showGizmo)) showGizmo = !showGizmo;
            if (ImGui::MenuItem("Game View [G]", nullptr, isGameView)) ToggleGameView(scene);
            if (ImGui::MenuItem("Immersive Viewport [F11]", nullptr, isImmersiveMode)) ToggleImmersiveMode();
            ImGui::EndPopup();
        }

        ImGui::SameLine();
        ImGui::TextDisabled("| Cam: %.1f", camera.moveSpeed);

        ImGui::SameLine();
        ImGui::TextDisabled("| FPS: %.1f (%.1fms)", fps, frameTimeMs);
    }
    ImGui::End();
    ImGui::PopStyleVar();

    // 3D Viewport Point Light Sprite Billboards (dev.md)
    if (!isGameView) {
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        float aspect = vpRect.width / vpRect.height;
        glm::mat4 proj = camera.GetProjectionMatrix(aspect);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 vpMatrix = proj * view;

        bool mouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        ImVec2 mousePos = ImGui::GetMousePos();

        for (auto& obj : scene.objects) {
            if (!obj.visible || !obj.isLight) continue;
            glm::vec3 worldPos = scene.GetWorldPosition(obj);
            glm::vec4 clip = vpMatrix * glm::vec4(worldPos, 1.0f);
            if (clip.w <= 0.05f) continue;

            glm::vec3 ndc = glm::vec3(clip) / clip.w;
            if (ndc.z < -1.0f || ndc.z > 1.0f) continue;

            float screenX = vp->Pos.x + vpRect.x + (ndc.x * 0.5f + 0.5f) * vpRect.width;
            float screenY = vp->Pos.y + vpRect.y + ((1.0f - ndc.y) * 0.5f) * vpRect.height;

            if (screenX < vp->Pos.x + vpRect.x || screenX > vp->Pos.x + vpRect.x + vpRect.width ||
                screenY < vp->Pos.y + vpRect.y || screenY > vp->Pos.y + vpRect.y + vpRect.height) continue;

            float iconSize = 38.0f;
            ImVec2 pMin(screenX - iconSize * 0.5f, screenY - iconSize * 0.5f);
            ImVec2 pMax(screenX + iconSize * 0.5f, screenY + iconSize * 0.5f);

            bool isSelected = (scene.selectedId == obj.id);

            // Draw Sprite Billboard
            if (lightIconGpuHandle != 0) {
                drawList->AddImage((ImTextureID)lightIconGpuHandle, pMin, pMax,
                                   ImVec2(0, 0), ImVec2(1, 1),
                                   isSelected ? IM_COL32(255, 255, 255, 255) : IM_COL32(230, 230, 230, 210));
            } else {
                drawList->AddCircleFilled(ImVec2(screenX, screenY), 12.0f, IM_COL32(255, 210, 70, 220));
            }

            // Selection ring
            if (isSelected) {
                drawList->AddCircle(ImVec2(screenX, screenY), 20.0f, IM_COL32(0, 255, 100, 255), 24, 2.0f);
                drawList->AddCircle(ImVec2(screenX, screenY), 22.0f, IM_COL32(255, 230, 80, 180), 24, 1.0f);
            } else {
                drawList->AddCircle(ImVec2(screenX, screenY), 16.0f, IM_COL32(255, 220, 60, 90), 16, 1.0f);
            }

            // Click detection on sprite
            if (mouseClicked && !ImGui::GetIO().WantCaptureMouse && !ImGuizmo::IsOver() && !ImGuizmo::IsUsing()) {
                float dx = mousePos.x - screenX;
                float dy = mousePos.y - screenY;
                if (dx * dx + dy * dy <= 20.0f * 20.0f) {
                    scene.selectedId = obj.id;
                    AddLog("LogActor", "Selected Light Actor via Viewport Sprite: " + obj.name, 0);
                }
            }
        }
    }

    // 1px border framing the center 3D viewport exactly according to UI_Ref.svg blueprint
    ImDrawList* fgDrawList = ImGui::GetForegroundDrawList();
    ImVec2 vpMin(vp->Pos.x + vpRect.x, vp->Pos.y + vpRect.y);
    ImVec2 vpMax(vp->Pos.x + vpRect.x + vpRect.width, vp->Pos.y + vpRect.y + vpRect.height);
    fgDrawList->AddRect(vpMin, vpMax, IM_COL32(40, 40, 40, 255), 0.0f, 0, 1.0f);
}

void EngineUI::DrawOutlinerNode(GameObject& obj, Scene& scene, std::unordered_set<int>& visitedIds, int depth) {
    if (depth > 64 || visitedIds.count(obj.id)) return;
    visitedIds.insert(obj.id);

    ImGui::PushID(obj.id);

    // Visibility eye
    if (ImGui::SmallButton(obj.visible ? "eye" : " - ")) {
        obj.visible = !obj.visible;
    }
    ImGui::SameLine();

    bool hasChildren = !obj.childIds.empty();
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (!hasChildren) {
        nodeFlags |= ImGuiTreeNodeFlags_Leaf;
    } else {
        nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
    }
    if (scene.selectedId == obj.id) {
        nodeFlags |= ImGuiTreeNodeFlags_Selected;
    }

    char label[128];
    if (obj.isLight || IsLightPrimitive(obj.type)) {
        snprintf(label, sizeof(label), "[%s] %s", GetLightTypeName(obj.light.type), obj.name.c_str());
    } else if (obj.type == PrimitiveType::Empty) {
        snprintf(label, sizeof(label), "[Empty] %s", obj.name.c_str());
    } else {
        snprintf(label, sizeof(label), "%s (%s)", obj.name.c_str(), GetPrimitiveTypeName(obj.type));
    }

    bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)obj.id, nodeFlags, "%s", label);
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        scene.selectedId = obj.id;
    }
    if (ImGui::BeginPopupContextItem()) {
        scene.selectedId = obj.id;
        if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
            GameObject* copy = scene.DuplicateObject(obj.id);
            if (copy) {
                std::string logMsg = "Duplicated Actor: " + copy->name;
                if (!copy->childIds.empty()) {
                    logMsg += " (along with " + std::to_string(copy->childIds.size()) + " children)";
                }
                AddLog("LogActor", logMsg, 2);
                EngineLogger::Get().LogAction("DUPLICATE_ACTOR", copy->name, "Children: " + std::to_string(copy->childIds.size()));
            }
        }
        if (ImGui::MenuItem("Delete", "Delete")) {
            scene.RemoveObject(obj.id);
        }
        ImGui::EndPopup();
    }

    // Drag source: drag this actor to reparent it
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
        int actorId = obj.id;
        ImGui::SetDragDropPayload("OUTLINER_ACTOR", &actorId, sizeof(int));
        ImGui::Text("Reparent %s", obj.name.c_str());
        ImGui::EndDragDropSource();
    }

    // Drag target: drop onto this actor to make dragged actor a child (deferred to end of outliner)
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("OUTLINER_ACTOR")) {
            int draggedId = *(const int*)payload->Data;
            if (draggedId != obj.id && !scene.IsDescendantOf(obj.id, draggedId)) {
                m_pendingReparentChild = draggedId;
                m_pendingReparentParent = obj.id;
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (nodeOpen) {
        if (hasChildren) {
            std::vector<int> childList = obj.childIds;
            for (int cid : childList) {
                GameObject* childObj = scene.FindObject(cid);
                if (childObj) {
                    DrawOutlinerNode(*childObj, scene, visitedIds, depth + 1);
                }
            }
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void EngineUI::RenderOutliner(Scene& scene) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    float margin = uiMargin;
    float gap = uiGap;

    float curBottomH = (showBottomDrawer ? (bottomDrawerOpen ? bottomDockHeight : 38.0f) : 0.0f);
    float bottomY = vp->Pos.y + vp->Size.y - curBottomH - margin;

    float sidebarsY = vp->Pos.y + margin + topBarHeight + gap;
    float sidebarsH = (curBottomH > 0.0f) ? (bottomY - gap - sidebarsY) : (vp->Pos.y + vp->Size.y - margin - sidebarsY);

    // Docked as Left Sidebar (matching UI_Ref.svg)
    ImGui::SetNextWindowPos(ImVec2(vp->Pos.x + margin, sidebarsY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(leftSidebarWidth, sidebarsH), ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    if (ImGui::Begin("Outliner", &showOutliner, flags)) {
        // Search bar
        ImGui::InputTextWithHint("##Filter", "Search Actors...", outlinerFilter, sizeof(outlinerFilter));
        ImGui::Separator();

        ImGui::TextDisabled("Level: MainWorld (%d actors)", (int)scene.objects.size());

        // Outliner Actor List Table
        ImGui::BeginChild("OutlinerList", ImVec2(0, -44), true);

        // If filtering, display matching flat list
        if (outlinerFilter[0] != '\0') {
            std::string filterStr = outlinerFilter;
            std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

            for (auto& obj : scene.objects) {
                std::string nameStr = obj.name;
                std::transform(nameStr.begin(), nameStr.end(), nameStr.begin(), ::tolower);
                if (nameStr.find(filterStr) == std::string::npos) continue;

                ImGui::PushID(obj.id);
                if (ImGui::SmallButton(obj.visible ? "eye" : " - ")) {
                    obj.visible = !obj.visible;
                }
                ImGui::SameLine();

                bool isSelected = (scene.selectedId == obj.id);
                char itemLabel[128];
                if (obj.isLight || IsLightPrimitive(obj.type)) snprintf(itemLabel, sizeof(itemLabel), "[%s] %s", GetLightTypeName(obj.light.type), obj.name.c_str());
                else if (obj.type == PrimitiveType::Empty) snprintf(itemLabel, sizeof(itemLabel), "[Empty] %s", obj.name.c_str());
                else snprintf(itemLabel, sizeof(itemLabel), "%s (%s)", obj.name.c_str(), GetPrimitiveTypeName(obj.type));

                if (ImGui::Selectable(itemLabel, isSelected)) {
                    scene.selectedId = obj.id;
                }
                ImGui::PopID();
            }
        } else {
            // Hierarchical tree view: render all root actors (parentId == -1)
            std::unordered_set<int> visitedIds;
            std::vector<int> rootIds;
            for (const auto& obj : scene.objects) {
                if (obj.parentId == -1) {
                    rootIds.push_back(obj.id);
                }
            }
            for (int rid : rootIds) {
                GameObject* rootObj = scene.FindObject(rid);
                if (rootObj) {
                    DrawOutlinerNode(*rootObj, scene, visitedIds, 0);
                }
            }
        }

        // Drop on Outliner empty space to unparent / move to root
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("OUTLINER_ACTOR")) {
                int draggedId = *(const int*)payload->Data;
                m_pendingReparentChild = draggedId;
                m_pendingReparentParent = -1;
            }
            ImGui::EndDragDropTarget();
        }

        // Right click context menu on outliner background
        if (ImGui::BeginPopupContextWindow("OutlinerContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
            if (ImGui::BeginMenu("3D Meshes / Primitives")) {
                if (ImGui::MenuItem("Cube"))      { scene.AddObject(PrimitiveType::Cube); AddLog("LogActor", "Spawned Cube", 2); }
                if (ImGui::MenuItem("Box"))       { scene.AddObject(PrimitiveType::Box); AddLog("LogActor", "Spawned Box", 2); }
                if (ImGui::MenuItem("Plane"))     { scene.AddObject(PrimitiveType::Plane); AddLog("LogActor", "Spawned Plane", 2); }
                if (ImGui::MenuItem("Sphere"))    { scene.AddObject(PrimitiveType::Sphere); AddLog("LogActor", "Spawned Sphere", 2); }
                if (ImGui::MenuItem("UV Sphere")) { scene.AddObject(PrimitiveType::UVSphere); AddLog("LogActor", "Spawned UV Sphere", 2); }
                if (ImGui::MenuItem("Icosphere")) { scene.AddObject(PrimitiveType::Icosphere); AddLog("LogActor", "Spawned Icosphere", 2); }
                if (ImGui::MenuItem("Cylinder"))  { scene.AddObject(PrimitiveType::Cylinder); AddLog("LogActor", "Spawned Cylinder", 2); }
                if (ImGui::MenuItem("Cone"))      { scene.AddObject(PrimitiveType::Cone); AddLog("LogActor", "Spawned Cone", 2); }
                if (ImGui::MenuItem("Capsule"))   { scene.AddObject(PrimitiveType::Capsule); AddLog("LogActor", "Spawned Capsule", 2); }
                if (ImGui::MenuItem("Torus"))     { scene.AddObject(PrimitiveType::Torus); AddLog("LogActor", "Spawned Torus", 2); }
                if (ImGui::MenuItem("Circle"))    { scene.AddObject(PrimitiveType::Circle); AddLog("LogActor", "Spawned Circle", 2); }
                if (ImGui::MenuItem("Disc"))      { scene.AddObject(PrimitiveType::Disc); AddLog("LogActor", "Spawned Disc", 2); }
                if (ImGui::MenuItem("Quad"))      { scene.AddObject(PrimitiveType::Quad); AddLog("LogActor", "Spawned Quad", 2); }
                if (ImGui::MenuItem("Triangle"))  { scene.AddObject(PrimitiveType::Triangle); AddLog("LogActor", "Spawned Triangle", 2); }
                if (ImGui::MenuItem("Pyramid"))   { scene.AddObject(PrimitiveType::Pyramid); AddLog("LogActor", "Spawned Pyramid", 2); }
                if (ImGui::MenuItem("Prism"))     { scene.AddObject(PrimitiveType::Prism); AddLog("LogActor", "Spawned Prism", 2); }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Lights")) {
                if (ImGui::MenuItem("Directional Light")) { scene.AddNewLight(PrimitiveType::DirectionalLight); AddLog("LogActor", "Spawned Directional Light", 2); }
                if (ImGui::MenuItem("Point Light"))       { scene.AddNewLight(PrimitiveType::PointLight); AddLog("LogActor", "Spawned Point Light", 2); }
                if (ImGui::MenuItem("Spot Light"))        { scene.AddNewLight(PrimitiveType::SpotLight); AddLog("LogActor", "Spawned Spot Light", 2); }
                if (ImGui::MenuItem("Area Light"))        { scene.AddNewLight(PrimitiveType::AreaLight); AddLog("LogActor", "Spawned Area Light", 2); }
                if (ImGui::MenuItem("Sky Light"))         { scene.AddNewLight(PrimitiveType::SkyLight); AddLog("LogActor", "Spawned Sky Light", 2); }
                if (ImGui::MenuItem("Ambient Light"))     { scene.AddNewLight(PrimitiveType::AmbientLight); AddLog("LogActor", "Spawned Ambient Light", 2); }
                if (ImGui::MenuItem("Hemisphere Light"))  { scene.AddNewLight(PrimitiveType::HemisphereLight); AddLog("LogActor", "Spawned Hemisphere Light", 2); }
                if (ImGui::MenuItem("Tube Light"))        { scene.AddNewLight(PrimitiveType::TubeLight); AddLog("LogActor", "Spawned Tube Light", 2); }
                if (ImGui::MenuItem("Disc Light"))        { scene.AddNewLight(PrimitiveType::DiscLight); AddLog("LogActor", "Spawned Disc Light", 2); }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Empty Actor")) { scene.AddEmptyActor(); AddLog("LogActor", "Spawned Empty Actor", 2); }
            ImGui::EndPopup();
        }

        ImGui::EndChild();

        // Bottom Actions
        if (ImGui::Button("+ Add Actor")) {
            ImGui::OpenPopup("AddActorOutlinerPopup");
        }
        if (ImGui::BeginPopup("AddActorOutlinerPopup")) {
            if (ImGui::BeginMenu("3D Meshes / Primitives")) {
                if (ImGui::MenuItem("Cube"))      { scene.AddObject(PrimitiveType::Cube); AddLog("LogActor", "Spawned Cube", 2); }
                if (ImGui::MenuItem("Box"))       { scene.AddObject(PrimitiveType::Box); AddLog("LogActor", "Spawned Box", 2); }
                if (ImGui::MenuItem("Plane"))     { scene.AddObject(PrimitiveType::Plane); AddLog("LogActor", "Spawned Plane", 2); }
                if (ImGui::MenuItem("Sphere"))    { scene.AddObject(PrimitiveType::Sphere); AddLog("LogActor", "Spawned Sphere", 2); }
                if (ImGui::MenuItem("UV Sphere")) { scene.AddObject(PrimitiveType::UVSphere); AddLog("LogActor", "Spawned UV Sphere", 2); }
                if (ImGui::MenuItem("Icosphere")) { scene.AddObject(PrimitiveType::Icosphere); AddLog("LogActor", "Spawned Icosphere", 2); }
                if (ImGui::MenuItem("Cylinder"))  { scene.AddObject(PrimitiveType::Cylinder); AddLog("LogActor", "Spawned Cylinder", 2); }
                if (ImGui::MenuItem("Cone"))      { scene.AddObject(PrimitiveType::Cone); AddLog("LogActor", "Spawned Cone", 2); }
                if (ImGui::MenuItem("Capsule"))   { scene.AddObject(PrimitiveType::Capsule); AddLog("LogActor", "Spawned Capsule", 2); }
                if (ImGui::MenuItem("Torus"))     { scene.AddObject(PrimitiveType::Torus); AddLog("LogActor", "Spawned Torus", 2); }
                if (ImGui::MenuItem("Circle"))    { scene.AddObject(PrimitiveType::Circle); AddLog("LogActor", "Spawned Circle", 2); }
                if (ImGui::MenuItem("Disc"))      { scene.AddObject(PrimitiveType::Disc); AddLog("LogActor", "Spawned Disc", 2); }
                if (ImGui::MenuItem("Quad"))      { scene.AddObject(PrimitiveType::Quad); AddLog("LogActor", "Spawned Quad", 2); }
                if (ImGui::MenuItem("Triangle"))  { scene.AddObject(PrimitiveType::Triangle); AddLog("LogActor", "Spawned Triangle", 2); }
                if (ImGui::MenuItem("Pyramid"))   { scene.AddObject(PrimitiveType::Pyramid); AddLog("LogActor", "Spawned Pyramid", 2); }
                if (ImGui::MenuItem("Prism"))     { scene.AddObject(PrimitiveType::Prism); AddLog("LogActor", "Spawned Prism", 2); }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Lights")) {
                if (ImGui::MenuItem("Directional Light")) { scene.AddNewLight(PrimitiveType::DirectionalLight); AddLog("LogActor", "Spawned Directional Light", 2); }
                if (ImGui::MenuItem("Point Light"))       { scene.AddNewLight(PrimitiveType::PointLight); AddLog("LogActor", "Spawned Point Light", 2); }
                if (ImGui::MenuItem("Spot Light"))        { scene.AddNewLight(PrimitiveType::SpotLight); AddLog("LogActor", "Spawned Spot Light", 2); }
                if (ImGui::MenuItem("Area Light"))        { scene.AddNewLight(PrimitiveType::AreaLight); AddLog("LogActor", "Spawned Area Light", 2); }
                if (ImGui::MenuItem("Sky Light"))         { scene.AddNewLight(PrimitiveType::SkyLight); AddLog("LogActor", "Spawned Sky Light", 2); }
                if (ImGui::MenuItem("Ambient Light"))     { scene.AddNewLight(PrimitiveType::AmbientLight); AddLog("LogActor", "Spawned Ambient Light", 2); }
                if (ImGui::MenuItem("Hemisphere Light"))  { scene.AddNewLight(PrimitiveType::HemisphereLight); AddLog("LogActor", "Spawned Hemisphere Light", 2); }
                if (ImGui::MenuItem("Tube Light"))        { scene.AddNewLight(PrimitiveType::TubeLight); AddLog("LogActor", "Spawned Tube Light", 2); }
                if (ImGui::MenuItem("Disc Light"))        { scene.AddNewLight(PrimitiveType::DiscLight); AddLog("LogActor", "Spawned Disc Light", 2); }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Empty Actor")) { scene.AddEmptyActor(); AddLog("LogActor", "Spawned Empty Actor via Outliner", 2); }
            ImGui::EndPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("+ Light")) {
            ImGui::OpenPopup("AddLightPopup");
        }
        if (ImGui::BeginPopup("AddLightPopup")) {
            if (ImGui::MenuItem("Directional Light")) { scene.AddNewLight(PrimitiveType::DirectionalLight); AddLog("LogActor", "Spawned Directional Light", 2); }
            if (ImGui::MenuItem("Point Light"))       { scene.AddNewLight(PrimitiveType::PointLight); AddLog("LogActor", "Spawned Point Light", 2); }
            if (ImGui::MenuItem("Spot Light"))        { scene.AddNewLight(PrimitiveType::SpotLight); AddLog("LogActor", "Spawned Spot Light", 2); }
            if (ImGui::MenuItem("Area Light"))        { scene.AddNewLight(PrimitiveType::AreaLight); AddLog("LogActor", "Spawned Area Light", 2); }
            if (ImGui::MenuItem("Sky Light"))         { scene.AddNewLight(PrimitiveType::SkyLight); AddLog("LogActor", "Spawned Sky Light", 2); }
            if (ImGui::MenuItem("Ambient Light"))     { scene.AddNewLight(PrimitiveType::AmbientLight); AddLog("LogActor", "Spawned Ambient Light", 2); }
            if (ImGui::MenuItem("Hemisphere Light"))  { scene.AddNewLight(PrimitiveType::HemisphereLight); AddLog("LogActor", "Spawned Hemisphere Light", 2); }
            if (ImGui::MenuItem("Tube Light"))        { scene.AddNewLight(PrimitiveType::TubeLight); AddLog("LogActor", "Spawned Tube Light", 2); }
            if (ImGui::MenuItem("Disc Light"))        { scene.AddNewLight(PrimitiveType::DiscLight); AddLog("LogActor", "Spawned Disc Light", 2); }
            ImGui::EndPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("+ Empty")) {
            scene.AddEmptyActor();
            AddLog("LogActor", "Spawned Empty Actor via Outliner", 2);
        }

        ImGui::SameLine();
        if (ImGui::Button("Duplicate") && scene.selectedId != -1) {
            GameObject* copy = scene.DuplicateObject(scene.selectedId);
            if (copy) {
                std::string logMsg = "Duplicated Actor: " + copy->name;
                if (!copy->childIds.empty()) {
                    logMsg += " (along with " + std::to_string(copy->childIds.size()) + " children)";
                }
                AddLog("LogActor", logMsg, 2);
                EngineLogger::Get().LogAction("DUPLICATE_ACTOR", copy->name, "Children: " + std::to_string(copy->childIds.size()));
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete") && scene.selectedId != -1) {
            scene.RemoveObject(scene.selectedId);
        }

        // Process any deferred reparent request after all outliner nodes and tree pops have cleanly finished
        if (m_pendingReparentChild != -1) {
            int childId = m_pendingReparentChild;
            int parentId = m_pendingReparentParent;
            m_pendingReparentChild = -1;
            m_pendingReparentParent = -1;

            GameObject* child = scene.FindObject(childId);
            std::string childName = child ? child->name : ("Actor " + std::to_string(childId));

            if (parentId != -1) {
                GameObject* parent = scene.FindObject(parentId);
                std::string parentName = parent ? parent->name : ("Actor " + std::to_string(parentId));
                scene.ReparentObject(childId, parentId);
                AddLog("LogActor", "Reparented '" + childName + "' into '" + parentName + "'", 0);
                EngineLogger::Get().LogAction("REPARENT_ACTOR", childName, "Parent: " + parentName);
            } else {
                scene.ReparentObject(childId, -1);
                AddLog("LogActor", "Detached '" + childName + "' to Root Level", 0);
                EngineLogger::Get().LogAction("UNPARENT_ACTOR", childName, "Moved to Root");
            }
        }
    }
    ImGui::End();
}

bool EngineUI::DrawTransformPill(const char* label, float& value, const glm::vec4& color, float resetValue, float speed) {
    bool modified = false;
    ImGui::PushID(label);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(color.r, color.g, color.b, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(color.r * 1.15f, color.g * 1.15f, color.b * 1.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(color.r * 0.9f, color.g * 0.9f, color.b * 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
    char btnLabel[32];
    snprintf(btnLabel, sizeof(btnLabel), "%s##btn_%s", label, label);
    if (ImGui::Button(btnLabel, ImVec2(24, 0))) {
        value = resetValue;
        modified = true;
    }
    ImGui::PopStyleColor(4);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 6.0f);
    char valLabel[32];
    snprintf(valLabel, sizeof(valLabel), "##val_%s", label);
    if (ImGui::DragFloat(valLabel, &value, speed, 0.0f, 0.0f, "%.2f")) {
        modified = true;
    }

    ImGui::PopID();
    return modified;
}

void EngineUI::RenderDetails(Scene& scene) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    float margin = uiMargin;
    float gap = uiGap;

    float curBottomH = (showBottomDrawer ? (bottomDrawerOpen ? bottomDockHeight : 38.0f) : 0.0f);
    float bottomY = vp->Pos.y + vp->Size.y - curBottomH - margin;

    float sidebarsY = vp->Pos.y + margin + topBarHeight + gap;
    float sidebarsH = (curBottomH > 0.0f) ? (bottomY - gap - sidebarsY) : (vp->Pos.y + vp->Size.y - margin - sidebarsY);

    // Docked as Right Sidebar (matching UI_Ref.svg)
    ImGui::SetNextWindowPos(ImVec2(vp->Pos.x + vp->Size.x - rightSidebarWidth - margin, sidebarsY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(rightSidebarWidth, sidebarsH), ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    if (ImGui::Begin("Details", &showDetails, flags)) {
        GameObject* obj = scene.GetSelected();
        if (!obj) {
            ImGui::TextDisabled("Select an Actor in the Outliner to view details.");
            ImGui::End();
            return;
        }

        ImGui::PushID(obj->id);

        // Actor Header Banner
        if (obj->isLight) {
            ImGui::TextColored(ImVec4(1.00f, 0.85f, 0.20f, 1.0f), "[PointLightComponent]");
        } else if (obj->type == PrimitiveType::Empty) {
            ImGui::TextColored(ImVec4(0.35f, 0.80f, 1.00f, 1.0f), "[EmptyActor]");
        } else {
            ImGui::TextColored(ImVec4(0.08f, 0.65f, 1.00f, 1.0f), "[StaticMeshActor]");
        }
        ImGui::SameLine();
        char nameBuf[128];
        strncpy(nameBuf, obj->name.c_str(), sizeof(nameBuf));
        nameBuf[sizeof(nameBuf) - 1] = '\0';
        if (ImGui::InputText("##ActorName", nameBuf, sizeof(nameBuf))) {
            obj->name = nameBuf;
            if (obj->isLight && obj->lightId >= 0 && obj->lightId < (int)scene.pointLights.size()) {
                scene.pointLights[obj->lightId].name = obj->name;
            }
        }

        // Unreal Mobility Selector (Static | Stationary | Movable)
        ImGui::TextDisabled("Mobility");
        ImGui::SameLine();
        int mIdx = (int)obj->mobility;
        const char* mobilities[] = { "Static", "Stationary", "Movable" };
        for (int i = 0; i < 3; ++i) {
            bool active = (mIdx == i);
            if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.50f, 0.90f, 1.0f));
            if (ImGui::Button(mobilities[i])) {
                obj->mobility = (Mobility)i;
            }
            if (active) ImGui::PopStyleColor();
            if (i < 2) ImGui::SameLine();
        }

        ImGui::Separator();

        // 1. Transform Category (Unreal Style X=Red, Y=Green, Z=Blue pills)
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool transformChanged = false;
            // Location
            ImGui::PushID("Location");
            ImGui::Text("Location");
            transformChanged |= DrawTransformPill("X", obj->position.x, glm::vec4(0.85f, 0.22f, 0.22f, 1.0f), 0.0f);
            transformChanged |= DrawTransformPill("Y", obj->position.y, glm::vec4(0.25f, 0.75f, 0.25f, 1.0f), 0.0f);
            transformChanged |= DrawTransformPill("Z", obj->position.z, glm::vec4(0.22f, 0.50f, 0.90f, 1.0f), 0.0f);
            ImGui::PopID();

            // Rotation
            ImGui::PushID("Rotation");
            ImGui::Spacing();
            ImGui::Text("Rotation");
            transformChanged |= DrawTransformPill("X", obj->rotation.x, glm::vec4(0.85f, 0.22f, 0.22f, 1.0f), 0.0f, 1.0f);
            transformChanged |= DrawTransformPill("Y", obj->rotation.y, glm::vec4(0.25f, 0.75f, 0.25f, 1.0f), 0.0f, 1.0f);
            transformChanged |= DrawTransformPill("Z", obj->rotation.z, glm::vec4(0.22f, 0.50f, 0.90f, 1.0f), 0.0f, 1.0f);
            ImGui::PopID();

            // Scale
            ImGui::PushID("Scale");
            ImGui::Spacing();
            ImGui::Text("Scale");
            float prevScaleX = obj->scale.x;
            if (DrawTransformPill("X", obj->scale.x, glm::vec4(0.85f, 0.22f, 0.22f, 1.0f), 1.0f, 0.02f) && lockAspectScale) {
                float ratio = (prevScaleX > 0.001f) ? (obj->scale.x / prevScaleX) : 1.0f;
                obj->scale.y *= ratio;
                obj->scale.z *= ratio;
                transformChanged = true;
            }
            transformChanged |= DrawTransformPill("Y", obj->scale.y, glm::vec4(0.25f, 0.75f, 0.25f, 1.0f), 1.0f, 0.02f);
            transformChanged |= DrawTransformPill("Z", obj->scale.z, glm::vec4(0.22f, 0.50f, 0.90f, 1.0f), 1.0f, 0.02f);
            ImGui::Checkbox("Lock Uniform Scale", &lockAspectScale);
            ImGui::PopID();

            if (transformChanged && (obj->isLight || IsLightPrimitive(obj->type))) {
                scene.SyncLightPositionsFromActors();
            }
        }

        // Light Component Category (dev.md)
        if (obj->isLight || IsLightPrimitive(obj->type)) {
            char lightHeader[128];
            snprintf(lightHeader, sizeof(lightHeader), "%s Component", GetLightTypeName(obj->light.type));
            if (ImGui::CollapsingHeader(lightHeader, ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "[%s]", GetLightTypeName(obj->light.type));
                ImGui::SameLine();
                ImGui::Checkbox("Light Enabled", &obj->light.enabled);

                ImGui::Separator();
                ImGui::TextDisabled("Light Settings");

                if (ImGui::ColorEdit3("Color##LightComponentColor", &obj->light.color.r,
                                      ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_DisplayRGB)) {
                    obj->color = obj->light.color;
                    if (obj->lightId >= 0 && obj->lightId < (int)scene.pointLights.size()) {
                        scene.pointLights[obj->lightId].color = obj->light.color;
                    }
                    if (obj->light.type == LightType::Directional) {
                        scene.lightColor = obj->light.color;
                    }
                }

                ImGui::DragFloat("Intensity##LightIntensity", &obj->light.intensity, 0.05f, 0.0f, 200.0f, "%.2f");

                ImGui::Checkbox("Use Temperature (Kelvin)##UseTemp", &obj->light.useTemperature);
                if (obj->light.useTemperature) {
                    ImGui::SliderFloat("Temperature##LightTemp", &obj->light.temperature, 1000.0f, 12000.0f, "%.0f K");
                    glm::vec3 kelvinCol = ColorTemperatureToRGB(obj->light.temperature);
                    ImGui::SameLine();
                    ImGui::ColorButton("##KelvinPreview", ImVec4(kelvinCol.r, kelvinCol.g, kelvinCol.b, 1.0f),
                                       ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker, ImVec2(20, 20));
                    ImGui::TextDisabled("Combined Tint: R:%.2f G:%.2f B:%.2f",
                                        kelvinCol.r * obj->light.color.r,
                                        kelvinCol.g * obj->light.color.g,
                                        kelvinCol.b * obj->light.color.b);
                }

                if (obj->light.type != LightType::Directional && obj->light.type != LightType::Ambient && obj->light.type != LightType::Sky) {
                    ImGui::DragFloat("Range / Radius", &obj->light.range, 0.1f, 0.1f, 500.0f, "%.1f m");
                    ImGui::SliderFloat("Attenuation Exp", &obj->light.attenuation, 0.5f, 4.0f, "%.2f");
                }

                if (obj->light.type == LightType::Spot) {
                    ImGui::Separator();
                    ImGui::TextDisabled("Spot Cone Settings");
                    ImGui::SliderFloat("Inner Cone Angle", &obj->light.innerConeAngle, 0.0f, 89.0f, "%.1f deg");
                    if (obj->light.outerConeAngle < obj->light.innerConeAngle) obj->light.outerConeAngle = obj->light.innerConeAngle;
                    ImGui::SliderFloat("Outer Cone Angle", &obj->light.outerConeAngle, obj->light.innerConeAngle, 90.0f, "%.1f deg");
                    ImGui::SliderFloat("Cone Falloff", &obj->light.coneFalloff, 0.1f, 5.0f, "%.2f");
                }

                if (obj->light.type == LightType::Area) {
                    ImGui::Separator();
                    ImGui::TextDisabled("Area Light Shape");
                    const char* areaShapes[] = { "Rectangle", "Disk", "Sphere", "Tube" };
                    ImGui::Combo("Shape", &obj->light.areaShape, areaShapes, 4);
                    if (obj->light.areaShape == 0) {
                        ImGui::DragFloat("Width", &obj->light.width, 0.05f, 0.01f, 50.0f, "%.2f m");
                        ImGui::DragFloat("Height", &obj->light.height, 0.05f, 0.01f, 50.0f, "%.2f m");
                    } else if (obj->light.areaShape == 1 || obj->light.areaShape == 2) {
                        ImGui::DragFloat("Radius", &obj->light.radius, 0.05f, 0.01f, 25.0f, "%.2f m");
                    } else if (obj->light.areaShape == 3) {
                        ImGui::DragFloat("Length", &obj->light.length, 0.05f, 0.01f, 50.0f, "%.2f m");
                        ImGui::DragFloat("Radius", &obj->light.radius, 0.05f, 0.01f, 10.0f, "%.2f m");
                    }
                    ImGui::Checkbox("Two Sided", &obj->light.twoSided);
                }

                if (obj->light.type == LightType::Tube) {
                    ImGui::Separator();
                    ImGui::TextDisabled("Tube Dimensions");
                    ImGui::DragFloat("Length", &obj->light.length, 0.05f, 0.01f, 50.0f, "%.2f m");
                    ImGui::DragFloat("Radius", &obj->light.radius, 0.05f, 0.01f, 10.0f, "%.2f m");
                }

                if (obj->light.type == LightType::Disc) {
                    ImGui::Separator();
                    ImGui::TextDisabled("Disc Dimensions");
                    ImGui::DragFloat("Radius", &obj->light.radius, 0.05f, 0.01f, 25.0f, "%.2f m");
                    ImGui::Checkbox("Two Sided", &obj->light.twoSided);
                }

                if (obj->light.type == LightType::Hemisphere) {
                    ImGui::Separator();
                    ImGui::TextDisabled("Hemisphere Colors");
                    ImGui::ColorEdit3("Sky Color##HemiSkyCol", &obj->light.skyColor.r,
                                      ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_DisplayRGB);
                    ImGui::ColorEdit3("Ground Color##HemiGndCol", &obj->light.groundColor.r,
                                      ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_DisplayRGB);
                }

                if (obj->light.type == LightType::Sky) {
                    ImGui::Separator();
                    ImGui::TextDisabled("Environment & Sky");
                    char envBuf[260];
                    strncpy(envBuf, obj->light.envMapTexture.c_str(), sizeof(envBuf));
                    if (ImGui::InputText("Environment HDRI##SkyHDRI", envBuf, sizeof(envBuf))) {
                        obj->light.envMapTexture = envBuf;
                    }
                    ImGui::SliderFloat("HDRI Rotation##SkyRot", &obj->light.envRotation, 0.0f, 360.0f, "%.1f deg");
                    ImGui::SliderFloat("Diffuse Contribution##SkyDiff", &obj->light.diffuseContribution, 0.0f, 2.0f);
                    ImGui::SliderFloat("Specular Contribution##SkySpec", &obj->light.specularContribution, 0.0f, 2.0f);
                    ImGui::SliderFloat("Ambient Contribution##SkyAmb", &obj->light.ambientContribution, 0.0f, 2.0f);
                    ImGui::SliderFloat("Mip Level##SkyMip", &obj->light.mipLevel, 0.0f, 8.0f);
                    ImGui::ColorEdit3("Lower Hemisphere Color##SkyLowerHemiCol", &obj->light.lowerHemisphereColor.r,
                                      ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_DisplayRGB);
                }

                if (obj->light.type == LightType::Directional || obj->light.type == LightType::Point || obj->light.type == LightType::Spot) {
                    ImGui::Separator();
                    ImGui::TextDisabled("Shadows");
                    ImGui::Checkbox("Cast Shadows", &obj->light.castShadows);
                    if (obj->light.castShadows) {
                        ImGui::SliderFloat("Shadow Strength", &obj->light.shadowStrength, 0.0f, 1.0f);
                        ImGui::DragFloat("Shadow Bias", &obj->light.shadowBias, 0.0001f, 0.00001f, 0.05f, "%.5f");
                        const char* resOptions[] = { "512", "1024", "2048", "4096" };
                        int curResIdx = (obj->light.shadowResolution >= 4096) ? 3 : (obj->light.shadowResolution >= 2048) ? 2 : (obj->light.shadowResolution >= 1024) ? 1 : 0;
                        if (ImGui::Combo("Shadow Resolution", &curResIdx, resOptions, 4)) {
                            int resVals[] = { 512, 1024, 2048, 4096 };
                            obj->light.shadowResolution = resVals[curResIdx];
                        }
                        ImGui::DragFloat("Shadow Distance", &obj->light.shadowDistance, 1.0f, 5.0f, 1000.0f, "%.1f m");
                    }
                }

                ImGui::Separator();
                ImGui::TextDisabled("Volumetric Lighting");
                ImGui::Checkbox("Volumetric Scattering", &obj->light.volumetric);
                if (obj->light.volumetric) {
                    ImGui::SliderFloat("Scattering", &obj->light.volumetricScattering, 0.0f, 1.0f);
                    ImGui::DragFloat("Volumetric Intensity", &obj->light.volumetricIntensity, 0.05f, 0.0f, 10.0f);
                }

                ImGui::Separator();
                ImGui::TextDisabled("Optimization & Channels");
                int layer = (int)obj->light.lightLayer;
                if (ImGui::InputInt("Light Layer / Channel", &layer)) {
                    obj->light.lightLayer = (uint32_t)std::max(0, layer);
                }
            }

            if (obj->lightId >= 0 && obj->lightId < (int)scene.pointLights.size()) {
                scene.pointLights[obj->lightId].color = obj->light.color;
                scene.pointLights[obj->lightId].intensity = obj->light.intensity;
                scene.pointLights[obj->lightId].range = obj->light.range;
                scene.pointLights[obj->lightId].enabled = obj->light.enabled;
                scene.pointLights[obj->lightId].castShadows = obj->light.castShadows;
            }
        }

        // Primitive Shape Settings Category (dev.md)
        if (!obj->isLight && !IsLightPrimitive(obj->type) && obj->type != PrimitiveType::Empty && obj->type != PrimitiveType::ImportedMesh) {
            char shapeHeader[128];
            snprintf(shapeHeader, sizeof(shapeHeader), "%s Parameters", GetPrimitiveTypeName(obj->type));
            if (ImGui::CollapsingHeader(shapeHeader, ImGuiTreeNodeFlags_DefaultOpen)) {
                bool paramChanged = false;

                if (obj->type == PrimitiveType::Cube) {
                    paramChanged |= ImGui::DragFloat("Size", &obj->params.size, 0.05f, 0.05f, 20.0f, "%.2f");
                    paramChanged |= ImGui::SliderInt("Subdivisions", &obj->params.segmentsX, 1, 64);
                }
                else if (obj->type == PrimitiveType::Plane) {
                    paramChanged |= ImGui::DragFloat("Width", &obj->params.width, 0.1f, 0.1f, 100.0f, "%.2f");
                    paramChanged |= ImGui::DragFloat("Depth", &obj->params.depth, 0.1f, 0.1f, 100.0f, "%.2f");
                    paramChanged |= ImGui::SliderInt("Segments X", &obj->params.segmentsX, 1, 64);
                    paramChanged |= ImGui::SliderInt("Segments Z", &obj->params.segmentsZ, 1, 64);
                }
                else if (obj->type == PrimitiveType::Box) {
                    paramChanged |= ImGui::DragFloat("Width", &obj->params.width, 0.05f, 0.05f, 50.0f, "%.2f");
                    paramChanged |= ImGui::DragFloat("Height", &obj->params.height, 0.05f, 0.05f, 50.0f, "%.2f");
                    paramChanged |= ImGui::DragFloat("Depth", &obj->params.depth, 0.05f, 0.05f, 50.0f, "%.2f");
                    paramChanged |= ImGui::SliderInt("Segments X", &obj->params.segmentsX, 1, 32);
                    paramChanged |= ImGui::SliderInt("Segments Y", &obj->params.segmentsY, 1, 32);
                    paramChanged |= ImGui::SliderInt("Segments Z", &obj->params.segmentsZ, 1, 32);
                }
                else if (obj->type == PrimitiveType::Sphere || obj->type == PrimitiveType::UVSphere) {
                    paramChanged |= ImGui::DragFloat("Radius", &obj->params.radius, 0.05f, 0.05f, 25.0f, "%.2f");
                    paramChanged |= ImGui::SliderInt("Rings / Lat", &obj->params.rings, 3, 64);
                    paramChanged |= ImGui::SliderInt("Sectors / Long", &obj->params.sectors, 3, 64);
                }
                else if (obj->type == PrimitiveType::Icosphere) {
                    paramChanged |= ImGui::DragFloat("Radius", &obj->params.radius, 0.05f, 0.05f, 25.0f, "%.2f");
                    paramChanged |= ImGui::SliderInt("Subdivisions", &obj->params.subdivisions, 0, 5);
                }
                else if (obj->type == PrimitiveType::Cylinder) {
                    paramChanged |= ImGui::DragFloat("Radius", &obj->params.radius, 0.05f, 0.05f, 25.0f, "%.2f");
                    paramChanged |= ImGui::DragFloat("Height", &obj->params.height, 0.05f, 0.05f, 50.0f, "%.2f");
                    paramChanged |= ImGui::SliderInt("Radial Segments", &obj->params.radialSegments, 3, 64);
                    paramChanged |= ImGui::SliderInt("Height Segments", &obj->params.heightSegments, 1, 32);
                    paramChanged |= ImGui::Checkbox("Cap Top", &obj->params.capTop);
                    paramChanged |= ImGui::Checkbox("Cap Bottom", &obj->params.capBottom);
                }
                else if (obj->type == PrimitiveType::Cone) {
                    paramChanged |= ImGui::DragFloat("Bottom Radius", &obj->params.radius, 0.05f, 0.0f, 25.0f, "%.2f");
                    paramChanged |= ImGui::DragFloat("Top Radius", &obj->params.radius2, 0.05f, 0.0f, 25.0f, "%.2f");
                    paramChanged |= ImGui::DragFloat("Height", &obj->params.height, 0.05f, 0.05f, 50.0f, "%.2f");
                    paramChanged |= ImGui::SliderInt("Radial Segments", &obj->params.radialSegments, 3, 64);
                    paramChanged |= ImGui::SliderInt("Height Segments", &obj->params.heightSegments, 1, 32);
                    paramChanged |= ImGui::Checkbox("Cap Bottom", &obj->params.capBottom);
                    paramChanged |= ImGui::Checkbox("Cap Top", &obj->params.capTop);
                }
                else if (obj->type == PrimitiveType::Capsule) {
                    paramChanged |= ImGui::DragFloat("Radius", &obj->params.radius, 0.05f, 0.05f, 20.0f, "%.2f");
                    paramChanged |= ImGui::DragFloat("Height", &obj->params.height, 0.05f, 0.05f, 40.0f, "%.2f");
                    paramChanged |= ImGui::SliderInt("Radial Segments", &obj->params.radialSegments, 3, 48);
                    paramChanged |= ImGui::SliderInt("Height Segments", &obj->params.heightSegments, 1, 24);
                }
                else if (obj->type == PrimitiveType::Torus) {
                    paramChanged |= ImGui::DragFloat("Major Radius", &obj->params.radius, 0.05f, 0.1f, 25.0f, "%.2f");
                    paramChanged |= ImGui::DragFloat("Minor Radius", &obj->params.radius2, 0.02f, 0.02f, 10.0f, "%.2f");
                    paramChanged |= ImGui::SliderInt("Major Segments", &obj->params.radialSegments, 3, 64);
                    paramChanged |= ImGui::SliderInt("Minor Segments", &obj->params.segmentsY, 3, 48);
                }
                else if (obj->type == PrimitiveType::Circle || obj->type == PrimitiveType::Disc) {
                    paramChanged |= ImGui::DragFloat("Radius", &obj->params.radius, 0.05f, 0.05f, 25.0f, "%.2f");
                    paramChanged |= ImGui::SliderInt("Segments", &obj->params.radialSegments, 3, 64);
                }
                else if (obj->type == PrimitiveType::Quad) {
                    paramChanged |= ImGui::DragFloat("Width", &obj->params.width, 0.05f, 0.05f, 50.0f, "%.2f");
                    paramChanged |= ImGui::DragFloat("Height", &obj->params.height, 0.05f, 0.05f, 50.0f, "%.2f");
                }
                else if (obj->type == PrimitiveType::Triangle) {
                    paramChanged |= ImGui::DragFloat("Base Width", &obj->params.width, 0.05f, 0.05f, 50.0f, "%.2f");
                    paramChanged |= ImGui::DragFloat("Height", &obj->params.height, 0.05f, 0.05f, 50.0f, "%.2f");
                }
                else if (obj->type == PrimitiveType::Pyramid) {
                    paramChanged |= ImGui::DragFloat("Base Width", &obj->params.width, 0.05f, 0.05f, 50.0f, "%.2f");
                    paramChanged |= ImGui::DragFloat("Base Depth", &obj->params.depth, 0.05f, 0.05f, 50.0f, "%.2f");
                    paramChanged |= ImGui::DragFloat("Height", &obj->params.height, 0.05f, 0.05f, 50.0f, "%.2f");
                    paramChanged |= ImGui::SliderInt("Sides", &obj->params.sides, 3, 32);
                }
                else if (obj->type == PrimitiveType::Prism) {
                    paramChanged |= ImGui::DragFloat("Radius", &obj->params.radius, 0.05f, 0.05f, 25.0f, "%.2f");
                    paramChanged |= ImGui::DragFloat("Height", &obj->params.height, 0.05f, 0.05f, 50.0f, "%.2f");
                    paramChanged |= ImGui::SliderInt("Sides", &obj->params.sides, 3, 32);
                }

                if (paramChanged) {
                    obj->RebuildMesh();
                }
            }
        }

        // Empty Actor Information
        if (obj->type == PrimitiveType::Empty) {
            ImGui::Spacing();
            ImGui::TextDisabled("Empty Actor (Transform Pivot / Group Node)");
            ImGui::TextDisabled("Has no visual mesh. Children inherit transform.");
        }

        // 2. Static Mesh Category (only for mesh actors)
        if (!obj->isLight && obj->type != PrimitiveType::Empty && ImGui::CollapsingHeader("Static Mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
            char meshAsset[64];
            snprintf(meshAsset, sizeof(meshAsset), "SM_%s", GetPrimitiveTypeName(obj->type));
            ImGui::TextDisabled("Static Mesh Asset:");
            ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.0f), "%s", meshAsset);
            ImGui::TextDisabled("Vertices: %d | Triangles: %d", (int)obj->mesh.vertices.size(), (int)obj->mesh.indices.size() / 3);
        }

        // 3. Materials Category (Element 0) (only for mesh actors)
        if (!obj->isLight && obj->type != PrimitiveType::Empty && ImGui::CollapsingHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextDisabled("Element 0: Material Slot");

            // Scan for all materials in contentRootPath (recursively across the entire project root)
            struct FoundMat {
                std::string name;        // e.g. "Default_Material"
                std::string relPath;     // e.g. "Materials/Default_Material.emat"
                std::string relFolder;   // e.g. "Materials", or "" for project root
                std::string fullPath;    // Full absolute path on disk
                glm::vec3 color{0.55f, 0.55f, 0.55f};
                float metallic = 0.0f;
                float roughness = 0.5f;
            };

            static std::vector<FoundMat> s_cachedMatList;
            static std::map<std::string, std::vector<FoundMat>> s_cachedFolderGroups;
            static auto s_lastMatScanTime = std::chrono::steady_clock::time_point::min();
            auto now = std::chrono::steady_clock::now();
            auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastMatScanTime).count();

            if (elapsedMs >= 2000 || s_cachedMatList.empty()) {
                s_lastMatScanTime = now;
                s_cachedMatList.clear();
                s_cachedFolderGroups.clear();

                std::error_code matEc;
                if (std::filesystem::exists(contentRootPath, matEc)) {
                    auto options = std::filesystem::directory_options::skip_permission_denied;
                    for (auto it = std::filesystem::recursive_directory_iterator(contentRootPath, options, matEc);
                         !matEc && it != std::filesystem::recursive_directory_iterator();
                         it.increment(matEc)) {
                        if (!it->is_directory(matEc)) {
                            std::string ext = it->path().extension().string();
                            if (ext == ".emat" || ext == ".mat") {
                                FoundMat fm;
                                fm.name = it->path().stem().string();
                                fm.fullPath = it->path().string();

                                std::filesystem::path rel = std::filesystem::relative(it->path(), contentRootPath, matEc);
                                if (!matEc) {
                                    fm.relPath = rel.generic_string();
                                    std::string parentFolder = rel.parent_path().generic_string();
                                    fm.relFolder = (parentFolder == ".") ? "" : parentFolder;
                                } else {
                                    fm.relPath = it->path().filename().string();
                                    fm.relFolder = "";
                                }

                                MaterialAsset ma;
                                if (LoadMaterialFile(fm.fullPath, ma)) {
                                    fm.color = ma.baseColor;
                                    fm.metallic = ma.metallic;
                                    fm.roughness = ma.roughness;
                                }

                                s_cachedMatList.push_back(fm);
                                s_cachedFolderGroups[fm.relFolder].push_back(fm);
                            }
                        }
                    }
                }

                // Ensure Default_Material is always available
                if (s_cachedMatList.empty()) {
                    std::filesystem::path defPath = contentRootPath / "Materials" / "Default_Material.emat";
                    FoundMat fm;
                    fm.name = "Default_Material";
                    fm.relPath = "Materials/Default_Material.emat";
                    fm.relFolder = "Materials";
                    fm.fullPath = defPath.string();
                    fm.color = glm::vec3(0.55f, 0.55f, 0.55f);
                    s_cachedMatList.push_back(fm);
                    s_cachedFolderGroups[fm.relFolder].push_back(fm);
                }
            }

            const auto& matList = s_cachedMatList;
            const auto& folderGroups = s_cachedFolderGroups;

            // Construct combo preview label
            std::string comboPreview = "🎨 " + (obj->materialName.empty() ? std::string("Default_Material") : obj->materialName);
            for (const auto& m : matList) {
                if (m.name == obj->materialName || m.relPath == obj->materialName) {
                    if (!m.relFolder.empty()) {
                        comboPreview += "  [" + m.relFolder + "]";
                    } else {
                        comboPreview += "  [Root]";
                    }
                    break;
                }
            }

            // Recurrent Material Dropdown Combo
            static char matFilter[64] = "";
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 36.0f);
            if (ImGui::BeginCombo("##MaterialSlotDropdown", comboPreview.c_str(), ImGuiComboFlags_HeightLarge)) {
                // Search filter inside dropdown
                ImGui::SetNextItemWidth(-1.0f);
                ImGui::InputTextWithHint("##matFilter", "🔍 Search materials in project...", matFilter, sizeof(matFilter));
                ImGui::Separator();

                std::string searchFilter = matFilter;
                std::transform(searchFilter.begin(), searchFilter.end(), searchFilter.begin(), ::tolower);

                if (!searchFilter.empty()) {
                    // Filtered view across all materials
                    int matchCount = 0;
                    for (const auto& m : matList) {
                        std::string nameLower = m.name;
                        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                        std::string relLower = m.relPath;
                        std::transform(relLower.begin(), relLower.end(), relLower.begin(), ::tolower);

                        if (nameLower.find(searchFilter) != std::string::npos || relLower.find(searchFilter) != std::string::npos) {
                            matchCount++;
                            ImGui::PushID(m.fullPath.c_str());
                            bool isSelected = (obj->materialName == m.name || obj->materialName == m.relPath);

                            ImGui::ColorButton("##swatch", ImVec4(m.color.r, m.color.g, m.color.b, 1.0f),
                                ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker, ImVec2(16, 16));
                            ImGui::SameLine();

                            std::string itemLabel = m.name;
                            if (!m.relFolder.empty()) {
                                itemLabel += "  (" + m.relFolder + ")";
                            } else {
                                itemLabel += "  (Root)";
                            }

                            if (ImGui::Selectable(itemLabel.c_str(), isSelected)) {
                                obj->materialName = m.name;
                                MaterialAsset ma;
                                if (LoadMaterialFile(m.fullPath, ma)) {
                                    obj->color = ma.baseColor;
                                    obj->metallic = ma.metallic;
                                    obj->roughness = ma.roughness;
                                    obj->normalStrength = ma.normalStrength;
                                    obj->specular = ma.specular;
                                    obj->emissiveColor = ma.emissiveColor;
                                    obj->emissiveIntensity = ma.emissiveIntensity;
                                    obj->shadingModel = ma.shadingModel;
                                    obj->blendMode = ma.blendMode;
                                    obj->twoSided = ma.twoSided;
                                    obj->castShadows = ma.castShadows;
                                    obj->receiveShadows = ma.receiveShadows;
                                    obj->baseColorTexture = ma.baseColorTexture;
                                    obj->normalTexture = ma.normalTexture;
                                    obj->roughnessTexture = ma.roughnessTexture;
                                    obj->metallicTexture = ma.metallicTexture;
                                    obj->aoTexture = ma.aoTexture;
                                    obj->emissionTexture = ma.emissionTexture;
                                    AddLog("LogMaterial", "Assigned material '" + m.name + "' (" + m.relPath + ") to Actor '" + obj->name + "'", 2);
                                }
                            }
                            if (isSelected) ImGui::SetItemDefaultFocus();
                            if (ImGui::IsItemHovered()) {
                                ImGui::SetTooltip("File: %s\nPath: %s\nRoughness: %.2f | Metallic: %.2f",
                                    m.name.c_str(), m.relPath.c_str(), m.roughness, m.metallic);
                            }
                            ImGui::PopID();
                        }
                    }
                    if (matchCount == 0) {
                        ImGui::TextDisabled("No materials match \"%s\"", matFilter);
                    }
                } else {
                    // Recurrent / Hierarchical Folder Grouping View
                    for (const auto& [folder, items] : folderGroups) {
                        std::string folderTitle;
                        if (folder.empty()) {
                            folderTitle = "📁 [Project Root] (" + std::to_string(items.size()) + ")";
                        } else {
                            folderTitle = "📁 " + folder + " (" + std::to_string(items.size()) + ")";
                        }

                        bool groupOpen = ImGui::TreeNodeEx(folderTitle.c_str(),
                            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth);

                        if (groupOpen) {
                            for (const auto& m : items) {
                                ImGui::PushID(m.fullPath.c_str());
                                bool isSelected = (obj->materialName == m.name || obj->materialName == m.relPath);

                                ImGui::ColorButton("##swatch", ImVec4(m.color.r, m.color.g, m.color.b, 1.0f),
                                    ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker, ImVec2(16, 16));
                                ImGui::SameLine();

                                if (ImGui::Selectable(m.name.c_str(), isSelected)) {
                                    obj->materialName = m.name;
                                    MaterialAsset ma;
                                    if (LoadMaterialFile(m.fullPath, ma)) {
                                        obj->color = ma.baseColor;
                                        obj->metallic = ma.metallic;
                                        obj->roughness = ma.roughness;
                                        obj->normalStrength = ma.normalStrength;
                                        obj->specular = ma.specular;
                                        obj->emissiveColor = ma.emissiveColor;
                                        obj->emissiveIntensity = ma.emissiveIntensity;
                                        obj->shadingModel = ma.shadingModel;
                                        obj->blendMode = ma.blendMode;
                                        obj->twoSided = ma.twoSided;
                                        obj->castShadows = ma.castShadows;
                                        obj->receiveShadows = ma.receiveShadows;
                                        obj->baseColorTexture = ma.baseColorTexture;
                                        obj->normalTexture = ma.normalTexture;
                                        obj->roughnessTexture = ma.roughnessTexture;
                                        obj->metallicTexture = ma.metallicTexture;
                                        obj->aoTexture = ma.aoTexture;
                                        obj->emissionTexture = ma.emissionTexture;
                                        AddLog("LogMaterial", "Assigned material '" + m.name + "' (" + m.relPath + ") to Actor '" + obj->name + "'", 2);
                                    }
                                }
                                if (isSelected) ImGui::SetItemDefaultFocus();
                                if (ImGui::IsItemHovered()) {
                                    ImGui::SetTooltip("Material: %s\nLocation: %s\nRoughness: %.2f | Metallic: %.2f",
                                        m.name.c_str(), m.relPath.c_str(), m.roughness, m.metallic);
                                }
                                ImGui::PopID();
                            }
                            ImGui::TreePop();
                        }
                    }
                }

                ImGui::EndCombo();
            }

            // Drag & Drop Target for Material Slot (dev.md Section 34)
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_ID")) {
                    char assetIdBuf[40] = {};
                    memcpy(assetIdBuf, payload->Data, std::min((size_t)payload->DataSize, sizeof(assetIdBuf) - 1));
                    AssetID droppedId = AssetID::FromString(assetIdBuf);
                    AssetMetadata* meta = AssetRegistry::Get().Find(droppedId);
                    if (meta && meta->type == AssetType::Material) {
                        std::filesystem::path fullPath = contentRootPath / meta->sourcePath;
                        MaterialAsset ma;
                        if (LoadMaterialFile(fullPath.string(), ma)) {
                            obj->materialName = ma.name;
                            obj->color = ma.baseColor;
                            obj->metallic = ma.metallic;
                            obj->roughness = ma.roughness;
                            obj->normalStrength = ma.normalStrength;
                            obj->specular = ma.specular;
                            obj->emissiveColor = ma.emissiveColor;
                            obj->emissiveIntensity = ma.emissiveIntensity;
                            obj->shadingModel = ma.shadingModel;
                            obj->blendMode = ma.blendMode;
                            obj->twoSided = ma.twoSided;
                            obj->castShadows = ma.castShadows;
                            obj->receiveShadows = ma.receiveShadows;
                            obj->baseColorTexture = ma.baseColorTexture;
                            obj->normalTexture = ma.normalTexture;
                            obj->roughnessTexture = ma.roughnessTexture;
                            obj->metallicTexture = ma.metallicTexture;
                            obj->aoTexture = ma.aoTexture;
                            obj->emissionTexture = ma.emissionTexture;
                            AddLog("LogMaterial", "Assigned Material '" + ma.name + "' via Drag & Drop", 2);
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            // Button to open currently selected material in Material Editor
            ImGui::SameLine();
            if (ImGui::Button("✏##EditMatBtn")) {
                std::string targetPath = "";
                for (const auto& m : matList) {
                    if (m.name == obj->materialName || m.relPath == obj->materialName) {
                        targetPath = m.fullPath;
                        break;
                    }
                }
                if (targetPath.empty()) {
                    targetPath = (contentRootPath / "Materials" / "Default_Material.emat").string();
                }
                OpenMaterialEditor(targetPath);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Open in Material Editor");
            }

            ImGui::SameLine();
            if (ImGui::Button("🔍##MatRefBtn")) {
                AssetMetadata* meta = AssetRegistry::Get().FindByNameOrPath(obj->materialName);
                if (meta) {
                    OpenReferenceViewer(meta->id);
                } else {
                    OpenReferenceViewer();
                }
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("View Material in Asset Reference Viewer (dev.md Section 33)");
            }

            ImGui::Spacing();
            if (ImGui::Button("🎨 Open in Material Editor", ImVec2(ImGui::GetContentRegionAvail().x, 26.0f))) {
                std::string targetPath = "";
                for (const auto& m : matList) {
                    if (m.name == obj->materialName || m.relPath == obj->materialName) {
                        targetPath = m.fullPath;
                        break;
                    }
                }
                if (targetPath.empty()) {
                    targetPath = (contentRootPath / "Materials" / "Default_Material.emat").string();
                }
                OpenMaterialEditor(targetPath);
            }
            ImGui::TextDisabled("ℹ Material properties are edited in the Material Editor.");
        }

        // 4. Lighting & Shadows
        if (!obj->isLight && obj->type != PrimitiveType::Empty && ImGui::CollapsingHeader("Lighting & Shadows", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Cast Shadows", &obj->castShadows);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("When enabled, this actor casts shadows into the real-time shadow map.");
            }
            ImGui::SameLine();
            ImGui::Checkbox("Receive Shadows", &obj->receiveShadows);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("When enabled, this actor receives shadows from directional sun light.");
            }
        }

        // 5. Actor Behavior
        if (ImGui::CollapsingHeader("Actor Behavior")) {
            ImGui::Checkbox("Auto-Rotate (Movable Only)", &obj->autoRotate);
            if (obj->autoRotate) {
                ImGui::DragFloat3("Speed (deg/s)", &obj->autoRotateSpeed.x, 1.0f, -360.0f, 360.0f);
            }
        }

        // 6. Behaviours (dev.md Section 6, 7, 8, 9, 10, 11, 12, 13)
        RenderBehavioursSection(scene, obj);

        ImGui::PopID();
    }
    ImGui::End();
}

std::vector<TextureAssetEntry> EngineUI::ScanProjectTextures() {
    std::vector<TextureAssetEntry> textures;
    auto regTextures = AssetRegistry::Get().FindByType(AssetType::Texture);
    textures.reserve(regTextures.size());

    for (const auto& meta : regTextures) {
        TextureAssetEntry te;
        te.filename = std::filesystem::path(meta.sourcePath).filename().string();
        te.fullPath = (contentRootPath / meta.sourcePath).string();
        te.relPath = meta.sourcePath;
        te.relFolder = std::filesystem::path(meta.sourcePath).parent_path().generic_string();
        if (te.relFolder == ".") te.relFolder = "";
        te.assetId = meta.id;
        te.virtualPath = meta.virtualPath;
        textures.push_back(te);
    }

    std::sort(textures.begin(), textures.end(), [](const auto& a, const auto& b) {
        return a.virtualPath < b.virtualPath;
    });
    return textures;
}

bool EngineUI::DrawTextureSlot(const char* label, std::string& textureSlotValue, AssetID& textureSlotAssetId, const std::vector<TextureAssetEntry>& availableTextures) {
    bool changed = false;
    ImGui::PushID(label);

    ImGui::Text("%s", label);

    std::string previewText;
    if (textureSlotAssetId.IsValid()) {
        AssetMetadata* meta = AssetRegistry::Get().Find(textureSlotAssetId);
        if (meta) {
            previewText = meta->objectName + "  [" + meta->id.ToString().substr(0, 8) + "...]";
        } else {
            previewText = "[Missing: " + textureSlotAssetId.ToString().substr(0, 8) + "...]";
        }
    } else if (!textureSlotValue.empty()) {
        previewText = std::filesystem::path(textureSlotValue).filename().string();
    } else {
        previewText = "(None)";
    }

    float availW = ImGui::GetContentRegionAvail().x;
    float clearBtnW = 28.0f;
    float comboW = availW - clearBtnW - 8.0f;
    if (comboW < 80.0f) comboW = 80.0f;

    ImGui::SetNextItemWidth(comboW);
    std::string comboId = "##TexCombo_" + std::string(label);
    if (ImGui::BeginCombo(comboId.c_str(), previewText.c_str(), ImGuiComboFlags_HeightLarge)) {
        char searchId[64];
        snprintf(searchId, sizeof(searchId), "##TexSearch_%s", label);
        static std::map<std::string, std::string> textureSearchMap;
        std::string& currentSearch = textureSearchMap[label];
        char searchBuf[64];
        strncpy(searchBuf, currentSearch.c_str(), sizeof(searchBuf));
        searchBuf[sizeof(searchBuf) - 1] = '\0';

        ImGui::SetNextItemWidth(-1.0f);
        if (ImGui::InputTextWithHint(searchId, "🔍 Search textures or AssetID...", searchBuf, sizeof(searchBuf))) {
            currentSearch = searchBuf;
        }
        ImGui::Separator();

        bool isNoneSelected = !textureSlotAssetId.IsValid() && textureSlotValue.empty();
        if (ImGui::Selectable("(None)", isNoneSelected)) {
            textureSlotValue = "";
            textureSlotAssetId = AssetID::Null();
            changed = true;
        }
        if (isNoneSelected) ImGui::SetItemDefaultFocus();
        ImGui::Separator();

        std::string searchLower = currentSearch;
        std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

        int count = 0;
        for (const auto& tex : availableTextures) {
            std::string nameLower = tex.filename;
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
            std::string virtLower = tex.virtualPath;
            std::transform(virtLower.begin(), virtLower.end(), virtLower.begin(), ::tolower);
            std::string idStr = tex.assetId.ToString();
            std::string idLower = idStr;
            std::transform(idLower.begin(), idLower.end(), idLower.begin(), ::tolower);

            if (!searchLower.empty() &&
                nameLower.find(searchLower) == std::string::npos &&
                virtLower.find(searchLower) == std::string::npos &&
                idLower.find(searchLower) == std::string::npos) {
                continue;
            }

            count++;
            ImGui::PushID(tex.fullPath.c_str());
            bool isSelected = (textureSlotAssetId == tex.assetId || textureSlotValue == tex.filename || textureSlotValue == tex.relPath || textureSlotValue == tex.virtualPath);

            std::string itemText = "🖼 " + tex.filename;
            if (!tex.virtualPath.empty()) {
                itemText += "  (" + tex.virtualPath + ")";
            }

            if (ImGui::Selectable(itemText.c_str(), isSelected)) {
                textureSlotValue = tex.filename;
                textureSlotAssetId = tex.assetId;
                changed = true;
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Virtual: %s\nAssetID: %s\nDisk: %s",
                    tex.virtualPath.c_str(), idStr.c_str(), tex.relPath.c_str());
            }
            ImGui::PopID();
        }

        if (count == 0 && !availableTextures.empty()) {
            ImGui::TextDisabled("No textures match \"%s\"", currentSearch.c_str());
        } else if (availableTextures.empty()) {
            ImGui::TextDisabled("No textures registered in Asset Registry");
        }

        ImGui::EndCombo();
    }

    // Drag & Drop Target for Texture Slot (dev.md Section 34)
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_ID")) {
            char assetIdBuf[40] = {};
            memcpy(assetIdBuf, payload->Data, std::min((size_t)payload->DataSize, sizeof(assetIdBuf) - 1));
            AssetID droppedId = AssetID::FromString(assetIdBuf);
            AssetMetadata* meta = AssetRegistry::Get().Find(droppedId);
            if (meta && meta->type == AssetType::Texture) {
                textureSlotAssetId = droppedId;
                textureSlotValue = std::filesystem::path(meta->sourcePath).filename().string();
                changed = true;
                AddLog("LogMaterial", "Assigned Texture '" + meta->objectName + "' (" + meta->id.ToString().substr(0, 8) + ") to slot", 2);
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::SameLine();
    bool hasTex = textureSlotAssetId.IsValid() || !textureSlotValue.empty();
    if (!hasTex) ImGui::BeginDisabled();
    if (ImGui::Button("×", ImVec2(clearBtnW, 0))) {
        textureSlotValue = "";
        textureSlotAssetId = AssetID::Null();
        changed = true;
    }
    if (!hasTex) ImGui::EndDisabled();
    if (ImGui::IsItemHovered() && hasTex) {
        ImGui::SetTooltip("Clear texture slot");
    }

    ImGui::Spacing();
    ImGui::PopID();
    return changed;
}

bool EngineUI::LoadMaterialFile(const std::string& path, MaterialAsset& outMat) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    outMat.filePath = path;
    outMat.name = std::filesystem::path(path).stem().string();

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string key = line.substr(0, colon);
        std::string val = line.substr(colon + 1);
        while (!key.empty() && isspace((unsigned char)key.back())) key.pop_back();
        while (!key.empty() && isspace((unsigned char)key.front())) key.erase(key.begin());
        while (!val.empty() && isspace((unsigned char)val.back())) val.pop_back();
        while (!val.empty() && isspace((unsigned char)val.front())) val.erase(val.begin());

        if (key == "assetId") outMat.assetId = AssetID::FromString(val);
        else if (key == "virtualPath") outMat.virtualPath = val;
        else if (key == "name") outMat.name = val;
        else if (key == "baseColor") {
            std::stringstream ss(val);
            ss >> outMat.baseColor.r >> outMat.baseColor.g >> outMat.baseColor.b;
        }
        else if (key == "metallic") {
            try { outMat.metallic = std::stof(val); } catch (...) {}
        }
        else if (key == "roughness") {
            try { outMat.roughness = std::stof(val); } catch (...) {}
        }
        else if (key == "normalStrength") {
            try { outMat.normalStrength = std::stof(val); } catch (...) {}
        }
        else if (key == "specular") {
            try { outMat.specular = std::stof(val); } catch (...) {}
        }
        else if (key == "emissiveColor") {
            std::stringstream ss(val);
            ss >> outMat.emissiveColor.r >> outMat.emissiveColor.g >> outMat.emissiveColor.b;
        }
        else if (key == "emissiveIntensity" || key == "emissionPower") {
            try { outMat.emissiveIntensity = std::stof(val); } catch (...) {}
        }
        else if (key == "shadingModel" || key == "shader") {
            try { outMat.shadingModel = std::stoi(val); } catch (...) {}
        }
        else if (key == "blendMode") {
            try { outMat.blendMode = std::stoi(val); } catch (...) {}
        }
        else if (key == "twoSided") {
            outMat.twoSided = (val == "1" || val == "true");
        }
        else if (key == "castShadows") {
            outMat.castShadows = (val == "1" || val == "true");
        }
        else if (key == "receiveShadows") {
            outMat.receiveShadows = (val == "1" || val == "true");
        }
        else if (key == "baseColorAssetId") outMat.baseColorAssetId = AssetID::FromString(val);
        else if (key == "normalAssetId") outMat.normalAssetId = AssetID::FromString(val);
        else if (key == "roughnessAssetId") outMat.roughnessAssetId = AssetID::FromString(val);
        else if (key == "metallicAssetId") outMat.metallicAssetId = AssetID::FromString(val);
        else if (key == "aoAssetId") outMat.aoAssetId = AssetID::FromString(val);
        else if (key == "emissionAssetId") outMat.emissionAssetId = AssetID::FromString(val);
        else if (key == "baseColorTexture") outMat.baseColorTexture = val;
        else if (key == "normalTexture") outMat.normalTexture = val;
        else if (key == "roughnessTexture") outMat.roughnessTexture = val;
        else if (key == "metallicTexture") outMat.metallicTexture = val;
        else if (key == "aoTexture") outMat.aoTexture = val;
        else if (key == "emissionTexture") outMat.emissionTexture = val;
    }

    // Resolve AssetID if not present
    if (!outMat.assetId.IsValid()) {
        AssetMetadata* meta = AssetRegistry::Get().FindByNameOrPath(path);
        if (meta) {
            outMat.assetId = meta->id;
            outMat.virtualPath = meta->virtualPath;
        }
    }

    // Resolve texture AssetIDs if missing
    auto resolveTexId = [&](const std::string& texName, AssetID& outId) {
        if (!outId.IsValid() && !texName.empty() && texName != "none") {
            AssetMetadata* m = AssetRegistry::Get().FindByNameOrPath(texName);
            if (m) outId = m->id;
        }
    };
    resolveTexId(outMat.baseColorTexture, outMat.baseColorAssetId);
    resolveTexId(outMat.normalTexture, outMat.normalAssetId);
    resolveTexId(outMat.roughnessTexture, outMat.roughnessAssetId);
    resolveTexId(outMat.metallicTexture, outMat.metallicAssetId);
    resolveTexId(outMat.aoTexture, outMat.aoAssetId);
    resolveTexId(outMat.emissionTexture, outMat.emissionAssetId);

    return true;
}

bool EngineUI::SaveMaterialFile(const std::string& path, const MaterialAsset& mat) {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << "# Eunoia-Editor Material Asset\n";
    if (mat.assetId.IsValid()) {
        file << "assetId: " << mat.assetId.ToString() << "\n";
    }
    if (!mat.virtualPath.empty()) {
        file << "virtualPath: " << mat.virtualPath << "\n";
    }
    file << "name: " << mat.name << "\n";
    file << "baseColor: " << mat.baseColor.r << " " << mat.baseColor.g << " " << mat.baseColor.b << "\n";
    file << "metallic: " << mat.metallic << "\n";
    file << "roughness: " << mat.roughness << "\n";
    file << "normalStrength: " << mat.normalStrength << "\n";
    file << "specular: " << mat.specular << "\n";
    file << "emissiveColor: " << mat.emissiveColor.r << " " << mat.emissiveColor.g << " " << mat.emissiveColor.b << "\n";
    file << "emissiveIntensity: " << mat.emissiveIntensity << "\n";
    file << "shadingModel: " << mat.shadingModel << "\n";
    file << "blendMode: " << mat.blendMode << "\n";
    file << "twoSided: " << (mat.twoSided ? 1 : 0) << "\n";
    file << "castShadows: " << (mat.castShadows ? 1 : 0) << "\n";
    file << "receiveShadows: " << (mat.receiveShadows ? 1 : 0) << "\n";
    if (mat.baseColorAssetId.IsValid()) file << "baseColorAssetId: " << mat.baseColorAssetId.ToString() << "\n";
    file << "baseColorTexture: " << mat.baseColorTexture << "\n";
    if (mat.normalAssetId.IsValid()) file << "normalAssetId: " << mat.normalAssetId.ToString() << "\n";
    file << "normalTexture: " << mat.normalTexture << "\n";
    if (mat.roughnessAssetId.IsValid()) file << "roughnessAssetId: " << mat.roughnessAssetId.ToString() << "\n";
    file << "roughnessTexture: " << mat.roughnessTexture << "\n";
    if (mat.metallicAssetId.IsValid()) file << "metallicAssetId: " << mat.metallicAssetId.ToString() << "\n";
    file << "metallicTexture: " << mat.metallicTexture << "\n";
    if (mat.aoAssetId.IsValid()) file << "aoAssetId: " << mat.aoAssetId.ToString() << "\n";
    file << "aoTexture: " << mat.aoTexture << "\n";
    if (mat.emissionAssetId.IsValid()) file << "emissionAssetId: " << mat.emissionAssetId.ToString() << "\n";
    file << "emissionTexture: " << mat.emissionTexture << "\n";

    // Update Asset Registry (dev.md Section 7, 13)
    if (mat.assetId.IsValid()) {
        AssetMetadata* meta = AssetRegistry::Get().Find(mat.assetId);
        if (meta) {
            meta->dependencies.clear();
            if (mat.baseColorAssetId.IsValid()) AssetRegistry::Get().AddDependency(mat.assetId, mat.baseColorAssetId);
            if (mat.normalAssetId.IsValid()) AssetRegistry::Get().AddDependency(mat.assetId, mat.normalAssetId);
            if (mat.roughnessAssetId.IsValid()) AssetRegistry::Get().AddDependency(mat.assetId, mat.roughnessAssetId);
            if (mat.metallicAssetId.IsValid()) AssetRegistry::Get().AddDependency(mat.assetId, mat.metallicAssetId);
            if (mat.aoAssetId.IsValid()) AssetRegistry::Get().AddDependency(mat.assetId, mat.aoAssetId);
            if (mat.emissionAssetId.IsValid()) AssetRegistry::Get().AddDependency(mat.assetId, mat.emissionAssetId);
        }
    }
    return true;
}

void EngineUI::OpenMaterialEditor(const std::string& path) {
    if (LoadMaterialFile(path, activeMaterial)) {
        showMaterialEditor = true;
        materialDirty = false;
        AddLog("LogMaterial", "Opened Material Editor: " + activeMaterial.name, 0);
    } else {
        AddLog("LogMaterial", "Failed to load material from: " + path, 3);
    }
}

void EngineUI::RenderMaterialEditor(Scene& scene) {
    ImGui::SetNextWindowSize(ImVec2(800, 640), ImGuiCond_FirstUseEver);
    std::string winTitle = "🎨 Material Editor - " + activeMaterial.name + (materialDirty ? "*" : "") + "###MaterialEditorWin";
    if (ImGui::Begin(winTitle.c_str(), &showMaterialEditor, ImGuiWindowFlags_NoCollapse)) {
        // Toolbar
        if (ImGui::Button("💾 Save Material") || (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S))) {
            if (SaveMaterialFile(activeMaterial.filePath, activeMaterial)) {
                materialDirty = false;
                int updatedCount = 0;
                for (auto& obj : scene.objects) {
                    if (obj.materialName == activeMaterial.name || 
                        obj.materialName == activeMaterial.filePath ||
                        std::filesystem::path(activeMaterial.filePath).stem().string() == obj.materialName) {
                        obj.color = activeMaterial.baseColor;
                        obj.metallic = activeMaterial.metallic;
                        obj.roughness = activeMaterial.roughness;
                        obj.normalStrength = activeMaterial.normalStrength;
                        obj.specular = activeMaterial.specular;
                        obj.emissiveColor = activeMaterial.emissiveColor;
                        obj.emissiveIntensity = activeMaterial.emissiveIntensity;
                        obj.shadingModel = activeMaterial.shadingModel;
                        obj.blendMode = activeMaterial.blendMode;
                        obj.twoSided = activeMaterial.twoSided;
                        obj.castShadows = activeMaterial.castShadows;
                        obj.receiveShadows = activeMaterial.receiveShadows;
                        obj.baseColorTexture = activeMaterial.baseColorTexture;
                        obj.normalTexture = activeMaterial.normalTexture;
                        obj.roughnessTexture = activeMaterial.roughnessTexture;
                        obj.metallicTexture = activeMaterial.metallicTexture;
                        obj.aoTexture = activeMaterial.aoTexture;
                        obj.emissionTexture = activeMaterial.emissionTexture;
                        updatedCount++;
                    }
                }
                GameObject* sel = scene.GetSelected();
                if (sel && sel->materialName == activeMaterial.name) {
                    sel->color = activeMaterial.baseColor;
                    sel->metallic = activeMaterial.metallic;
                    sel->roughness = activeMaterial.roughness;
                    sel->normalStrength = activeMaterial.normalStrength;
                    sel->specular = activeMaterial.specular;
                    sel->emissiveColor = activeMaterial.emissiveColor;
                    sel->emissiveIntensity = activeMaterial.emissiveIntensity;
                    sel->shadingModel = activeMaterial.shadingModel;
                    sel->blendMode = activeMaterial.blendMode;
                    sel->twoSided = activeMaterial.twoSided;
                    sel->castShadows = activeMaterial.castShadows;
                    sel->receiveShadows = activeMaterial.receiveShadows;
                    sel->baseColorTexture = activeMaterial.baseColorTexture;
                    sel->normalTexture = activeMaterial.normalTexture;
                    sel->roughnessTexture = activeMaterial.roughnessTexture;
                    sel->metallicTexture = activeMaterial.metallicTexture;
                    sel->aoTexture = activeMaterial.aoTexture;
                    sel->emissionTexture = activeMaterial.emissionTexture;
                }
                AddLog("LogMaterial", "Saved Material to " + activeMaterial.filePath + " (Reflected in " + std::to_string(updatedCount) + " actors in viewport)", 2);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("🎯 Apply to Selected Actor")) {
            GameObject* sel = scene.GetSelected();
            if (sel) {
                sel->materialName = activeMaterial.name;
                sel->color = activeMaterial.baseColor;
                sel->metallic = activeMaterial.metallic;
                sel->roughness = activeMaterial.roughness;
                sel->normalStrength = activeMaterial.normalStrength;
                sel->specular = activeMaterial.specular;
                sel->emissiveColor = activeMaterial.emissiveColor;
                sel->emissiveIntensity = activeMaterial.emissiveIntensity;
                sel->shadingModel = activeMaterial.shadingModel;
                sel->blendMode = activeMaterial.blendMode;
                sel->twoSided = activeMaterial.twoSided;
                sel->castShadows = activeMaterial.castShadows;
                sel->receiveShadows = activeMaterial.receiveShadows;
                sel->baseColorTexture = activeMaterial.baseColorTexture;
                sel->normalTexture = activeMaterial.normalTexture;
                sel->roughnessTexture = activeMaterial.roughnessTexture;
                sel->metallicTexture = activeMaterial.metallicTexture;
                sel->aoTexture = activeMaterial.aoTexture;
                sel->emissionTexture = activeMaterial.emissionTexture;
                AddLog("LogMaterial", "Applied material '" + activeMaterial.name + "' to Actor '" + sel->name + "'", 2);
            } else {
                AddLog("LogMaterial", "No Actor currently selected to apply material.", 1);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("🔄 Revert")) {
            LoadMaterialFile(activeMaterial.filePath, activeMaterial);
            materialDirty = false;
        }

        ImGui::SameLine();
        ImGui::Checkbox("Live Viewport Sync", &liveMaterialViewportSync);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("When checked, parameter tweaks sync to viewport in real-time.");
        }

        ImGui::SameLine();
        ImGui::TextDisabled("| %s", activeMaterial.name.c_str());

        ImGui::Separator();

        // Scan available textures across project root
        std::vector<TextureAssetEntry> availableTextures = ScanProjectTextures();

        // Two-pane Layout (Left: MATERIAL PREVIEW, Right: SURFACE, VALUES, SETTINGS)
        float previewPaneW = 270.0f;
        ImGui::BeginChild("MatPreviewPane", ImVec2(previewPaneW, 0), true);

        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "MATERIAL PREVIEW");
        ImGui::Spacing();

        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 canvasSize(previewPaneW - 20.0f, previewPaneW - 20.0f);
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Invisible button to capture drag for light rotation
        ImGui::InvisibleButton("##PreviewCanvasBtn", canvasSize);
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
            matEditorLightAngle += dragDelta.x * 0.5f;
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
        }

        // Viewport background
        drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(18, 20, 25, 255), 6.0f);
        drawList->AddRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(45, 48, 56, 255), 6.0f);

        // 3D-shaded Sphere geometry
        ImVec2 center(canvasPos.x + canvasSize.x * 0.5f, canvasPos.y + canvasSize.y * 0.5f);
        float radius = (canvasSize.x * 0.5f) - 16.0f;
        glm::vec3 col = activeMaterial.baseColor;
        CachedTexture* previewAlbedo = TextureManager::Get().GetTexture(activeMaterial.baseColorTexture);
        if (previewAlbedo && previewAlbedo->valid) {
            col = previewAlbedo->averageColor;
        }

        float lightRad = glm::radians(matEditorLightAngle);
        glm::vec3 L = glm::normalize(glm::vec3(sinf(lightRad) * 0.7f, -cosf(lightRad) * 0.5f - 0.2f, 0.75f));
        glm::vec3 V = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 H = glm::normalize(L + V);

        // Render multi-pass PBR sphere shading
        // Pass 1: Dark ambient backdrop
        float aoDarken = activeMaterial.aoTexture.empty() ? 1.0f : 0.75f;
        drawList->AddCircleFilled(center, radius,
            IM_COL32((int)glm::clamp(col.r * 45 * aoDarken, 0.0f, 255.0f),
                     (int)glm::clamp(col.g * 45 * aoDarken, 0.0f, 255.0f),
                     (int)glm::clamp(col.b * 45 * aoDarken, 0.0f, 255.0f), 255), 64);

        if (activeMaterial.shadingModel == 1) {
            // Unlit Shading
            drawList->AddCircleFilled(center, radius,
                IM_COL32((int)glm::clamp(col.r * 255, 0.0f, 255.0f),
                         (int)glm::clamp(col.g * 255, 0.0f, 255.0f),
                         (int)glm::clamp(col.b * 255, 0.0f, 255.0f), 255), 64);
        } else {
            // Pass 2: Diffuse gradient towards light
            ImVec2 diffCenter(center.x - L.x * radius * 0.22f, center.y + L.y * radius * 0.22f);
            drawList->AddCircleFilled(diffCenter, radius * 0.88f,
                IM_COL32((int)glm::clamp(col.r * 180 * aoDarken, 0.0f, 255.0f),
                         (int)glm::clamp(col.g * 180 * aoDarken, 0.0f, 255.0f),
                         (int)glm::clamp(col.b * 180 * aoDarken, 0.0f, 255.0f), 255), 64);

            // Pass 3: Core lit disk
            ImVec2 coreCenter(center.x - L.x * radius * 0.38f, center.y + L.y * radius * 0.38f);
            drawList->AddCircleFilled(coreCenter, radius * 0.68f,
                IM_COL32((int)glm::clamp(col.r * 255, 0.0f, 255.0f),
                         (int)glm::clamp(col.g * 255, 0.0f, 255.0f),
                         (int)glm::clamp(col.b * 255, 0.0f, 255.0f), 255), 64);

            // Pass 4: Specular highlight (Roughness & Metallic scaled)
            float specRough = glm::clamp(activeMaterial.roughness, 0.05f, 1.0f);
            float specRadius = radius * glm::mix(0.32f, 0.04f, specRough);
            int specAlpha = (int)(255.0f * (1.0f - specRough * 0.65f));
            glm::vec3 specColor = glm::mix(glm::vec3(1.0f), col, activeMaterial.metallic);

            ImVec2 specCenter(center.x - H.x * radius * 0.52f, center.y + H.y * radius * 0.52f);
            drawList->AddCircleFilled(specCenter, specRadius,
                IM_COL32((int)(specColor.r * 255), (int)(specColor.g * 255), (int)(specColor.b * 255), specAlpha), 32);

            // Subtler inner reflection hotspot
            if (specRough < 0.5f) {
                drawList->AddCircleFilled(specCenter, specRadius * 0.35f,
                    IM_COL32(255, 255, 255, (int)(specAlpha * 0.9f)), 24);
            }
        }

        // Pass 5: Emissive glow ring
        if (activeMaterial.emissiveIntensity > 0.01f) {
            glm::vec3 emCol = activeMaterial.emissiveColor;
            float glowAlpha = glm::clamp(activeMaterial.emissiveIntensity * 0.25f, 0.2f, 1.0f);
            drawList->AddCircle(center, radius + 3.5f,
                IM_COL32((int)glm::clamp(emCol.r * 255, 0.0f, 255.0f),
                         (int)glm::clamp(emCol.g * 255, 0.0f, 255.0f),
                         (int)glm::clamp(emCol.b * 255, 0.0f, 255.0f), (int)(glowAlpha * 255)), 64, 4.0f);
        }

        // Sphere outline
        drawList->AddCircle(center, radius, IM_COL32(40, 42, 50, 255), 64, 1.5f);

        // Texture indicator badge overlaid
        if (!activeMaterial.baseColorTexture.empty()) {
            drawList->AddRectFilled(ImVec2(canvasPos.x + 8, canvasPos.y + canvasSize.y - 24),
                                    ImVec2(canvasPos.x + canvasSize.x - 8, canvasPos.y + canvasSize.y - 6),
                                    IM_COL32(0, 0, 0, 180), 4.0f);
            std::string texBadge = "Texture: " + activeMaterial.baseColorTexture;
            if (texBadge.length() > 28) texBadge = texBadge.substr(0, 25) + "...";
            drawList->AddText(ImVec2(canvasPos.x + 12, canvasPos.y + canvasSize.y - 22),
                              IM_COL32(100, 220, 255, 255), texBadge.c_str());
        }

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "◯ 3D Sphere");
        ImGui::TextDisabled("Light: %.0f° (Drag sphere to orbit)", matEditorLightAngle);
        ImGui::TextDisabled("Shader: %s", activeMaterial.shadingModel == 0 ? "PBR Cook-Torrance" : (activeMaterial.shadingModel == 1 ? "Unlit" : "Subsurface"));

        ImGui::EndChild();

        ImGui::SameLine();

        // Right Pane: SURFACE, VALUES, SETTINGS (dev.md layout)
        ImGui::BeginChild("MatPropertiesPane", ImVec2(0, 0), true);

        char nameEdit[64];
        strncpy(nameEdit, activeMaterial.name.c_str(), sizeof(nameEdit));
        nameEdit[sizeof(nameEdit) - 1] = '\0';
        ImGui::Text("Material:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 10.0f);
        if (ImGui::InputText("##MatNameHeader", nameEdit, sizeof(nameEdit))) {
            activeMaterial.name = nameEdit;
            materialDirty = true;
        }

        if (activeMaterial.assetId.IsValid()) {
            ImGui::TextDisabled("AssetID: %s", activeMaterial.assetId.ToString().c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("📋 Copy##CopyMatAssetId")) {
                ImGui::SetClipboardText(activeMaterial.assetId.ToString().c_str());
                AddLog("LogMaterial", "Copied AssetID to clipboard: " + activeMaterial.assetId.ToString(), 0);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("🔍 View References##MatRefView")) {
                OpenReferenceViewer(activeMaterial.assetId);
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ═══════════════════════════════════════════
        // SECTION 1: SURFACE (dev.md)
        // ═══════════════════════════════════════════
        if (ImGui::CollapsingHeader("SURFACE", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Spacing();
            if (DrawTextureSlot("Base Color", activeMaterial.baseColorTexture, activeMaterial.baseColorAssetId, availableTextures)) {
                materialDirty = true;
                CachedTexture* tex = TextureManager::Get().GetTexture(activeMaterial.baseColorTexture);
                if (tex && tex->valid) {
                    activeMaterial.baseColor = tex->averageColor;
                }
            }
            if (DrawTextureSlot("Normal", activeMaterial.normalTexture, activeMaterial.normalAssetId, availableTextures)) {
                materialDirty = true;
            }
            if (DrawTextureSlot("Roughness", activeMaterial.roughnessTexture, activeMaterial.roughnessAssetId, availableTextures)) {
                materialDirty = true;
            }
            if (DrawTextureSlot("Metallic", activeMaterial.metallicTexture, activeMaterial.metallicAssetId, availableTextures)) {
                materialDirty = true;
            }
            if (DrawTextureSlot("Ambient Occlusion", activeMaterial.aoTexture, activeMaterial.aoAssetId, availableTextures)) {
                materialDirty = true;
            }
            if (DrawTextureSlot("Emission", activeMaterial.emissionTexture, activeMaterial.emissionAssetId, availableTextures)) {
                materialDirty = true;
            }
            ImGui::Spacing();
        }

        // ═══════════════════════════════════════════
        // SECTION 2: VALUES (dev.md)
        // ═══════════════════════════════════════════
        if (ImGui::CollapsingHeader("VALUES", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Spacing();
            if (ImGui::ColorEdit3("Base Color", &activeMaterial.baseColor.r, ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_DisplayRGB)) {
                materialDirty = true;
            }
            if (ImGui::SliderFloat("Metallic", &activeMaterial.metallic, 0.0f, 1.0f, "%.2f")) {
                materialDirty = true;
            }
            if (ImGui::SliderFloat("Roughness", &activeMaterial.roughness, 0.0f, 1.0f, "%.2f")) {
                materialDirty = true;
            }
            if (ImGui::SliderFloat("Normal Strength", &activeMaterial.normalStrength, 0.0f, 2.0f, "%.2f")) {
                materialDirty = true;
            }
            if (ImGui::ColorEdit3("Emission Color", &activeMaterial.emissiveColor.r)) {
                materialDirty = true;
            }
            if (ImGui::SliderFloat("Emission Power", &activeMaterial.emissiveIntensity, 0.0f, 30.0f, "%.2f")) {
                materialDirty = true;
            }
            ImGui::Spacing();
        }

        // ═══════════════════════════════════════════
        // SECTION 3: SETTINGS (dev.md)
        // ═══════════════════════════════════════════
        if (ImGui::CollapsingHeader("SETTINGS", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Spacing();
            const char* shaders[] = { "PBR", "Unlit", "Subsurface" };
            if (ImGui::Combo("Shader", &activeMaterial.shadingModel, shaders, IM_ARRAYSIZE(shaders))) {
                materialDirty = true;
            }

            const char* blendModes[] = { "Opaque", "Masked", "Translucent" };
            if (ImGui::Combo("Blend Mode", &activeMaterial.blendMode, blendModes, IM_ARRAYSIZE(blendModes))) {
                materialDirty = true;
            }

            if (ImGui::Checkbox("Two Sided", &activeMaterial.twoSided)) {
                materialDirty = true;
            }
            if (ImGui::Checkbox("Cast Shadows", &activeMaterial.castShadows)) {
                materialDirty = true;
            }
            if (ImGui::Checkbox("Receive Shadows", &activeMaterial.receiveShadows)) {
                materialDirty = true;
            }
            ImGui::Spacing();
        }

        // If Live Viewport Sync is enabled, update all scene actors in real-time
        if (liveMaterialViewportSync && materialDirty) {
            for (auto& obj : scene.objects) {
                if (obj.materialName == activeMaterial.name || 
                    obj.materialName == activeMaterial.filePath ||
                    std::filesystem::path(activeMaterial.filePath).stem().string() == obj.materialName) {
                    obj.color = activeMaterial.baseColor;
                    obj.metallic = activeMaterial.metallic;
                    obj.roughness = activeMaterial.roughness;
                    obj.normalStrength = activeMaterial.normalStrength;
                    obj.specular = activeMaterial.specular;
                    obj.emissiveColor = activeMaterial.emissiveColor;
                    obj.emissiveIntensity = activeMaterial.emissiveIntensity;
                    obj.shadingModel = activeMaterial.shadingModel;
                    obj.blendMode = activeMaterial.blendMode;
                    obj.twoSided = activeMaterial.twoSided;
                    obj.castShadows = activeMaterial.castShadows;
                    obj.receiveShadows = activeMaterial.receiveShadows;
                    obj.baseColorTexture = activeMaterial.baseColorTexture;
                    obj.normalTexture = activeMaterial.normalTexture;
                    obj.roughnessTexture = activeMaterial.roughnessTexture;
                    obj.metallicTexture = activeMaterial.metallicTexture;
                    obj.aoTexture = activeMaterial.aoTexture;
                    obj.emissionTexture = activeMaterial.emissionTexture;
                }
            }
        }

        ImGui::EndChild();
    }
    ImGui::End();
}

void EngineUI::RenderContentBrowser(Scene& scene) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    float margin = uiMargin;
    float curBottomH = bottomDrawerOpen ? bottomDockHeight : 38.0f;
    float bottomW = vp->Size.x - 2.0f * margin;
    float bottomY = vp->Pos.y + vp->Size.y - curBottomH - margin;

    // Docked as Full-Width Bottom Dock (matching UI_Ref.svg)
    ImGui::SetNextWindowPos(ImVec2(vp->Pos.x + margin, bottomY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(bottomW, curBottomH), ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    if (ImGui::Begin("Content Browser", &showBottomDrawer, flags)) {
        // Tab Header buttons
        if (bottomDrawerTab > 1) bottomDrawerTab = 0;
        bool isContentTab = (bottomDrawerTab == 0);
        bool isLogTab     = (bottomDrawerTab == 1);

        if (isContentTab) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.50f, 0.90f, 1.0f));
        if (ImGui::Button("📁 Content Browser")) bottomDrawerTab = 0;
        if (isContentTab) ImGui::PopStyleColor();

        ImGui::SameLine();
        if (isLogTab) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.50f, 0.90f, 1.0f));
        if (ImGui::Button("📋 Output Log")) bottomDrawerTab = 1;
        if (isLogTab) ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();
        if (ImGui::Button(bottomDrawerOpen ? "Collapse Dock" : "Expand Dock")) {
            bottomDrawerOpen = !bottomDrawerOpen;
        }
        ImGui::Separator();

        if (bottomDrawerOpen) {
            if (bottomDrawerTab == 0) {
                // Registry-Backed Content Browser (dev.md Section 14, 34, 35)
                std::error_code ec;
                if (currentVirtualDir.empty()) currentVirtualDir = "/Game";

                // Top Path Toolbar & Action Buttons
                bool isAtRoot = (currentVirtualDir == "/Game");
                if (isAtRoot) ImGui::BeginDisabled();
                if (ImGui::Button("⬆ Up")) {
                    currentVirtualDir = AssetPath::GetDirectory(currentVirtualDir);
                    selectedContentItem = "";
                }
                if (isAtRoot) ImGui::EndDisabled();

                ImGui::SameLine();
                if (ImGui::Button("🏠 /Game")) {
                    currentVirtualDir = "/Game";
                    selectedContentItem = "";
                }

                ImGui::SameLine();
                if (ImGui::Button("🔄 Sync Registry")) {
                    AssetRegistry::Get().ScanAndSync(contentRootPath);
                    AddLog("LogAsset", "Synchronized Asset Registry (" + std::to_string(AssetRegistry::Get().GetAssetCount()) + " assets)", 2);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Rescan project and synchronize Asset Registry");

                ImGui::SameLine();
                ImGui::TextDisabled("|");
                ImGui::SameLine();

                // Interactive Virtual Path Breadcrumbs
                std::string breadcrumb = currentVirtualDir;
                std::stringstream bss(breadcrumb);
                std::string seg;
                std::string accumVirtual = "";
                int segIdx = 0;
                while (std::getline(bss, seg, '/')) {
                    if (seg.empty()) continue;
                    accumVirtual += "/" + seg;
                    if (segIdx > 0) {
                        ImGui::SameLine();
                        ImGui::TextDisabled(">");
                        ImGui::SameLine();
                    }
                    ImGui::PushID(segIdx++);
                    bool isCurSeg = (accumVirtual == currentVirtualDir);
                    if (isCurSeg) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 1.0f, 1.0f));
                    if (ImGui::SmallButton(seg.c_str())) {
                        currentVirtualDir = accumVirtual;
                        selectedContentItem = "";
                    }
                    if (isCurSeg) ImGui::PopStyleColor();
                    ImGui::PopID();
                }

                // Action buttons on right
                float rightOffset = 540.0f;
                if (ImGui::GetContentRegionAvail().x > rightOffset) {
                    ImGui::SameLine(ImGui::GetWindowWidth() - rightOffset);
                } else {
                    ImGui::SameLine();
                }

                if (ImGui::Button("📥 Import Model")) {
                    ImportMeshWithSavePrompt(scene);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Import 3D model into project and level");
                ImGui::SameLine();

                if (ImGui::Button("➕ New Folder")) {
                    showNewFolderPopup = true;
                    snprintf(newFolderNameBuf, sizeof(newFolderNameBuf), "NewFolder");
                }
                ImGui::SameLine();
                if (ImGui::Button("🎨 New Material")) {
                    showNewMaterialPopup = true;
                    snprintf(newMaterialNameBuf, sizeof(newMaterialNameBuf), "M_NewMaterial");
                }
                ImGui::SameLine();
                if (ImGui::Button("🧩 New Behaviour")) {
                    showNewBehaviourPopup = true;
                    snprintf(newBehaviourNameBuf, sizeof(newBehaviourNameBuf), "PlayerController");
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Create a new EunoiaBehaviour C++ script asset (dev.md Section 4, 21)");
                ImGui::SameLine();
                if (ImGui::Button("📦 Cook")) {
                    OpenCookModal();
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Cook & Package Assets (dev.md Section 24, 25)");

                ImGui::SameLine();
                ImGui::SetNextItemWidth(125.0f);
                ImGui::InputTextWithHint("##CBSearch", "🔍 Search...", contentBrowserSearch, sizeof(contentBrowserSearch));

                ImGui::Separator();

                // Modals for creating Folder / Material
                if (showNewFolderPopup) {
                    ImGui::OpenPopup("Create Folder##Modal");
                    showNewFolderPopup = false;
                }
                if (ImGui::BeginPopupModal("Create Folder##Modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Enter virtual folder name:");
                    ImGui::SetNextItemWidth(260.0f);
                    ImGui::InputText("##FolderNameInput", newFolderNameBuf, sizeof(newFolderNameBuf));
                    ImGui::Spacing();
                    if (ImGui::Button("Create", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                        if (strlen(newFolderNameBuf) > 0) {
                            std::string diskSub = currentVirtualDir;
                            if (diskSub.rfind("/Game", 0) == 0) diskSub = diskSub.substr(5);
                            if (!diskSub.empty() && diskSub[0] == '/') diskSub.erase(0, 1);
                            std::filesystem::path newDiskDir = contentRootPath / diskSub / newFolderNameBuf;
                            std::error_code dirEc;
                            if (std::filesystem::create_directories(newDiskDir, dirEc)) {
                                AssetRegistry::Get().ScanAndSync(contentRootPath);
                                AddLog("LogContent", "Created folder: " + std::string(newFolderNameBuf), 2);
                            }
                        }
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                if (showNewMaterialPopup) {
                    ImGui::OpenPopup("Create Material##Modal");
                    showNewMaterialPopup = false;
                }
                if (ImGui::BeginPopupModal("Create Material##Modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Enter material asset name:");
                    ImGui::SetNextItemWidth(260.0f);
                    ImGui::InputText("##MatNameInput", newMaterialNameBuf, sizeof(newMaterialNameBuf));
                    ImGui::Spacing();
                    if (ImGui::Button("Create", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                        if (strlen(newMaterialNameBuf) > 0) {
                            std::string matFileName = newMaterialNameBuf;
                            if (matFileName.find(".emat") == std::string::npos) matFileName += ".emat";
                            std::string diskSub = currentVirtualDir;
                            if (diskSub.rfind("/Game", 0) == 0) diskSub = diskSub.substr(5);
                            if (!diskSub.empty() && diskSub[0] == '/') diskSub.erase(0, 1);
                            std::filesystem::path targetDir = contentRootPath / diskSub;
                            std::filesystem::create_directories(targetDir, ec);
                            std::filesystem::path matFilePath = targetDir / matFileName;

                            MaterialAsset newMat;
                            newMat.name = std::filesystem::path(matFileName).stem().string();
                            newMat.filePath = matFilePath.string();
                            newMat.assetId = AssetID::CreateRandom();
                            newMat.virtualPath = AssetPath::Combine(currentVirtualDir, newMat.name);
                            newMat.baseColor = glm::vec3(0.75f, 0.75f, 0.75f);
                            newMat.metallic = 0.0f;
                            newMat.roughness = 0.5f;
                            newMat.specular = 0.5f;

                            if (SaveMaterialFile(matFilePath.string(), newMat)) {
                                AssetRegistry::Get().ScanAndSync(contentRootPath);
                                AddLog("LogContent", "Created material asset: " + newMat.name + " (" + newMat.assetId.ToString() + ")", 2);
                                OpenMaterialEditor(matFilePath.string());
                            }
                        }
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                if (showNewBehaviourPopup) {
                    ImGui::OpenPopup("Create Behaviour##Modal");
                    showNewBehaviourPopup = false;
                }
                if (ImGui::BeginPopupModal("Create Behaviour##Modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Enter Behaviour Class Name (inherits EunoiaBehaviour):");
                    ImGui::SetNextItemWidth(280.0f);
                    ImGui::InputText("##BehNameInput", newBehaviourNameBuf, sizeof(newBehaviourNameBuf));
                    ImGui::Spacing();
                    if (ImGui::Button("Create", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                        if (strlen(newBehaviourNameBuf) > 0) {
                            std::string behName = newBehaviourNameBuf;
                            std::string diskSub = currentVirtualDir;
                            if (diskSub.rfind("/Game", 0) == 0) diskSub = diskSub.substr(5);
                            if (!diskSub.empty() && diskSub[0] == '/') diskSub.erase(0, 1);
                            std::filesystem::path behDir = contentRootPath / diskSub / "Behaviours";
                            std::error_code bec;
                            std::filesystem::create_directories(behDir, bec);

                            std::filesystem::path hPath = behDir / (behName + ".h");
                            std::filesystem::path cppPath = behDir / (behName + ".cpp");
                            std::filesystem::path metaPath = behDir / (behName + ".h.assetmeta");

                            std::ofstream hFile(hPath);
                            if (hFile.is_open()) {
                                hFile << "#pragma once\n#include \"EunoiaBehaviour.h\"\n\n"
                                      << "class " << behName << " : public EunoiaBehaviour {\npublic:\n"
                                      << "    float MoveSpeed = 5.0f;\n"
                                      << "    int Health = 100;\n"
                                      << "    bool IsActive = true;\n"
                                      << "    Light* WarningLight = nullptr;\n"
                                      << "    MeshRenderer* TargetMesh = nullptr;\n\n"
                                      << "    " << behName << "() {\n"
                                      << "        m_className = \"" << behName << "\";\n"
                                      << "        m_displayName = \"" << behName << "\";\n"
                                      << "        RegisterProperties();\n"
                                      << "    }\n\n"
                                      << "    void RegisterProperties() override {\n"
                                      << "        m_properties.clear();\n"
                                      << "        RegisterProperty(\"Move Speed\", &MoveSpeed, \"Movement\", 0.0f, 50.0f);\n"
                                      << "        RegisterProperty(\"Health\", &Health, \"Stats\", 0, 1000);\n"
                                      << "        RegisterProperty(\"Is Active\", &IsActive, \"General\");\n"
                                      << "        RegisterReference(\"Warning Light\", &WarningLight, ObjectRefType::Light, \"References\");\n"
                                      << "        RegisterReference(\"Target Mesh\", &TargetMesh, ObjectRefType::Mesh, \"References\");\n"
                                      << "    }\n\n"
                                      << "    std::unique_ptr<EunoiaBehaviour> Clone() const override {\n"
                                      << "        auto clone = std::make_unique<" << behName << ">(*this);\n"
                                      << "        clone->RegisterProperties();\n"
                                      << "        return clone;\n"
                                      << "    }\n\n"
                                      << "    void Start() override;\n"
                                      << "    void Update(float deltaTime) override;\n};\n";
                            }

                            std::ofstream cppFile(cppPath);
                            if (cppFile.is_open()) {
                                cppFile << "#include \"" << behName << ".h\"\n#include \"BehaviourRegistry.h\"\n#include \"GameObject.h\"\n\n"
                                        << "void " << behName << "::Start() {\n"
                                        << "    auto* light = GetRespectiveObject.Light(WarningLight);\n"
                                        << "    if (light) {\n        // Configure light\n    }\n"
                                        << "    AddEngineLog(\"LogBehaviour\", \"" << behName << "::Start called on \" + (GetOwner() ? GetOwner()->name : \"Unknown\"), 0);\n}\n\n"
                                        << "void " << behName << "::Update(float deltaTime) {\n"
                                        << "    // Custom gameplay logic for " << behName << "\n}\n";
                            }

                            std::ofstream metaFile(metaPath);
                            if (metaFile.is_open()) {
                                AssetID aid = AssetID::CreateRandom();
                                metaFile << "# Eunoia-Editor Asset Sidecar Metadata\n"
                                         << "assetId: " << aid.ToString() << "\n"
                                         << "type: Behaviour\n"
                                         << "baseClass: EunoiaBehaviour\n"
                                         << "virtualPath: " << currentVirtualDir << "/Behaviours/" << behName << "\n";
                            }

                            AssetRegistry::Get().ScanAndSync(contentRootPath);
                            AddLog("LogContent", "Created Behaviour asset: " + behName + " in " + currentVirtualDir + "/Behaviours/", 2);

                            // Open in code editor immediately
                            ShellExecuteA(NULL, "open", hPath.string().c_str(), NULL, NULL, SW_SHOWNORMAL);
                            OpenScriptInCodeEditor(hPath.string());
                        }
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                // Two-Pane Content Browser Layout (Left: Virtual Folders, Right: Assets)
                float leftFolderPaneW = 200.0f;
                ImGui::BeginChild("CBFolderTreePane", ImVec2(leftFolderPaneW, 0), true);
                ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "VIRTUAL FOLDERS");
                ImGui::Separator();

                // Virtual Directory Hierarchy
                auto RenderVirtualDirNode = [&](auto& self, const std::string& vDir) -> void {
                    std::vector<std::string> subdirs = AssetRegistry::Get().GetSubdirectories(vDir);
                    std::string dirName = AssetPath::GetObjectName(vDir);
                    if (vDir == "/Game") dirName = "Game";

                    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
                    if (vDir == currentVirtualDir) nodeFlags |= ImGuiTreeNodeFlags_Selected;
                    if (subdirs.empty()) nodeFlags |= ImGuiTreeNodeFlags_Leaf;
                    else nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;

                    bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)std::hash<std::string>{}(vDir), nodeFlags, "📁 %s", dirName.c_str());
                    if (ImGui::IsItemClicked()) {
                        currentVirtualDir = vDir;
                        selectedContentItem = "";
                    }
                    if (nodeOpen) {
                        for (const auto& sub : subdirs) {
                            self(self, sub);
                        }
                        ImGui::TreePop();
                    }
                };

                RenderVirtualDirNode(RenderVirtualDirNode, "/Game");
                ImGui::EndChild();

                ImGui::SameLine();

                // Right Pane: Asset Table backed by AssetRegistry (dev.md Section 14)
                ImGui::BeginChild("CBAssetListPane", ImVec2(0, 0), true);

                // Query assets in current virtual directory or search across registry
                std::string searchStr = contentBrowserSearch;
                std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

                std::vector<AssetMetadata> displayAssets;
                if (!searchStr.empty()) {
                    // Global search
                    if (searchStr.rfind("type:", 0) == 0) {
                        std::string typeFilter = searchStr.substr(5);
                        displayAssets = AssetRegistry::Get().FindByType(StringToAssetType(typeFilter));
                    } else if (searchStr.rfind("path:", 0) == 0) {
                        std::string pathFilter = searchStr.substr(5);
                        displayAssets = AssetRegistry::Get().FindInDirectory(pathFilter, true);
                    } else {
                        // Search by name, path, or AssetID
                        const auto& all = AssetRegistry::Get().GetAllAssets();
                        for (const auto& pair : all) {
                            const auto& m = pair.second;
                            std::string nLower = m.objectName;
                            std::transform(nLower.begin(), nLower.end(), nLower.begin(), ::tolower);
                            std::string pLower = m.virtualPath;
                            std::transform(pLower.begin(), pLower.end(), pLower.begin(), ::tolower);
                            std::string idLower = m.id.ToString();
                            std::transform(idLower.begin(), idLower.end(), idLower.begin(), ::tolower);

                            if (nLower.find(searchStr) != std::string::npos ||
                                pLower.find(searchStr) != std::string::npos ||
                                idLower.find(searchStr) != std::string::npos) {
                                displayAssets.push_back(m);
                            }
                        }
                    }
                } else {
                    displayAssets = AssetRegistry::Get().FindInDirectory(currentVirtualDir, false);
                }

                // Subdirectories in current virtual folder
                std::vector<std::string> currentSubdirs = AssetRegistry::Get().GetSubdirectories(currentVirtualDir);

                ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV |
                                             ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable |
                                             ImGuiTableFlags_ScrollY;

                if (ImGui::BeginTable("RegistryAssetTable", 5, tableFlags, ImVec2(0, 0))) {
                    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 110.0f);
                    ImGui::TableSetupColumn("Name & Virtual Path", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("AssetID", ImGuiTableColumnFlags_WidthFixed, 130.0f);
                    ImGui::TableSetupColumn("Deps / Refs", ImGuiTableColumnFlags_WidthFixed, 95.0f);
                    ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 75.0f);
                    ImGui::TableHeadersRow();

                    int rowIndex = 0;

                    // 1. Virtual Subdirectories (if no search)
                    if (searchStr.empty()) {
                        for (const auto& subDir : currentSubdirs) {
                            ImGui::TableNextRow();
                            ImGui::PushID(subDir.c_str());

                            std::string subName = AssetPath::GetObjectName(subDir);
                            bool isSelected = (selectedContentItem == subDir);

                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.35f, 1.0f), "📁 Folder");

                            ImGui::TableSetColumnIndex(1);
                            if (ImGui::Selectable(subName.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick)) {
                                selectedContentItem = subDir;
                                if (ImGui::IsMouseDoubleClicked(0)) {
                                    currentVirtualDir = subDir;
                                    selectedContentItem = "";
                                }
                            }
                            if (ImGui::IsItemHovered()) {
                                ImGui::SetTooltip("Virtual Folder: %s\n(Double-click to open)", subDir.c_str());
                            }

                            ImGui::TableSetColumnIndex(2);
                            ImGui::TextDisabled("-");
                            ImGui::TableSetColumnIndex(3);
                            ImGui::TextDisabled("-");
                            ImGui::TableSetColumnIndex(4);
                            ImGui::TextDisabled("Dir");

                            ImGui::PopID();
                            rowIndex++;
                        }
                    }

                    // 2. Assets from AssetRegistry
                    for (const auto& meta : displayAssets) {
                        ImGui::TableNextRow();
                        ImGui::PushID(meta.id.ToString().c_str());

                        bool isSelected = (selectedContentItem == meta.virtualPath);

                        // Column 0: Type with icon and color
                        ImGui::TableSetColumnIndex(0);
                        ImVec4 typeCol(0.7f, 0.7f, 0.7f, 1.0f);
                        if (meta.type == AssetType::Material) typeCol = ImVec4(0.35f, 0.85f, 1.0f, 1.0f);
                        else if (meta.type == AssetType::Texture) typeCol = ImVec4(0.40f, 0.90f, 0.50f, 1.0f);
                        else if (meta.type == AssetType::Mesh) typeCol = ImVec4(0.95f, 0.65f, 0.25f, 1.0f);
                        else if (meta.type == AssetType::Level) typeCol = ImVec4(0.30f, 0.85f, 0.95f, 1.0f);

                        ImGui::TextColored(typeCol, "%s %s", GetAssetTypeIcon(meta.type), AssetTypeToString(meta.type));

                        // Column 1: Object Name & Virtual Path
                        ImGui::TableSetColumnIndex(1);
                        if (ImGui::Selectable(meta.objectName.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick)) {
                            selectedContentItem = meta.virtualPath;
                            if (ImGui::IsMouseDoubleClicked(0)) {
                                if (meta.type == AssetType::Material) {
                                    std::filesystem::path fullMat = contentRootPath / meta.sourcePath;
                                    OpenMaterialEditor(fullMat.string());
                                } else if (meta.type == AssetType::Mesh) {
                                    std::filesystem::path fullMesh = contentRootPath / meta.sourcePath;
                                    GameObject& newObj = scene.AddImportedMesh(fullMesh.string(), glm::vec3(0.0f, 0.5f, 0.0f));
                                    scene.selectedId = newObj.id;
                                    AddLog("LogMesh", "Spawned 3D Mesh in level: " + meta.objectName, 2);
                                } else if (meta.type == AssetType::Level) {
                                    std::filesystem::path fullLevel = contentRootPath / meta.sourcePath;
                                    OpenLevelFromPath(scene, fullLevel.string());
                                } else if (meta.type == AssetType::Behaviour || meta.type == AssetType::Script) {
                                    std::filesystem::path fullScript = contentRootPath / meta.sourcePath;
                                    ShellExecuteA(NULL, "open", fullScript.string().c_str(), NULL, NULL, SW_SHOWNORMAL);
                                    OpenScriptInCodeEditor(fullScript.string());
                                } else {
                                    OpenReferenceViewer(meta.id);
                                }
                            }
                        }

                        // Drag & Drop Source transferring AssetID (dev.md Section 34)
                        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                            std::string idStr = meta.id.ToString();
                            ImGui::SetDragDropPayload("ASSET_ID", idStr.c_str(), idStr.length() + 1);
                            ImGui::Text("%s %s", GetAssetTypeIcon(meta.type), meta.objectName.c_str());
                            if (meta.type == AssetType::Mesh) {
                                ImGui::TextColored(ImVec4(0.95f, 0.65f, 0.25f, 1.0f), "[Drag to Viewport to spawn in Level]");
                            } else if (meta.type == AssetType::Level) {
                                ImGui::TextColored(ImVec4(0.30f, 0.85f, 0.95f, 1.0f), "[Drag to Viewport to open Level]");
                            }
                            ImGui::EndDragDropSource();
                        }

                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Object: %s\nVirtual: %s\nAssetID: %s\nDisk: %s\n(Drag & Drop into Material Slots | Double-click to open)",
                                meta.objectName.c_str(), meta.virtualPath.c_str(), meta.id.ToString().c_str(), meta.sourcePath.c_str());
                        }

                        // Column 2: AssetID (clickable copy)
                        ImGui::TableSetColumnIndex(2);
                        std::string shortId = meta.id.ToString().substr(0, 8) + "...";
                        if (ImGui::SmallButton(shortId.c_str())) {
                            ImGui::SetClipboardText(meta.id.ToString().c_str());
                            AddLog("LogAsset", "Copied AssetID to clipboard: " + meta.id.ToString(), 0);
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("AssetID: %s\n(Click to copy to clipboard)", meta.id.ToString().c_str());
                        }

                        // Column 3: Dependencies & References count
                        ImGui::TableSetColumnIndex(3);
                        size_t depCount = meta.dependencies.size();
                        size_t refCount = AssetRegistry::Get().GetReferences(meta.id).size();
                        char depRefStr[32];
                        snprintf(depRefStr, sizeof(depRefStr), "%d d / %d r##dr", (int)depCount, (int)refCount);
                        if (ImGui::SmallButton(depRefStr)) {
                            OpenReferenceViewer(meta.id);
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("%d Dependencies | %d References\n(Click to inspect in Reference Viewer)", (int)depCount, (int)refCount);
                        }

                        // Column 4: Status (Loaded / Available / Missing)
                        ImGui::TableSetColumnIndex(4);
                        if (meta.isMissing) {
                            ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.25f, 1.0f), "Missing");
                        } else if (AssetManager::Get().IsLoaded(meta.id)) {
                            ImGui::TextColored(ImVec4(0.3f, 0.95f, 0.4f, 1.0f), "Loaded");
                        } else {
                            ImGui::TextDisabled("Ready");
                        }

                        // Context Menu on Asset Item
                        if (ImGui::BeginPopupContextItem("AssetRowCtx")) {
                            if (meta.type == AssetType::Behaviour || meta.type == AssetType::Script) {
                                if (ImGui::MenuItem("💻 Open in Code Editor")) {
                                    std::filesystem::path fullScript = contentRootPath / meta.sourcePath;
                                    ShellExecuteA(NULL, "open", fullScript.string().c_str(), NULL, NULL, SW_SHOWNORMAL);
                                    OpenScriptInCodeEditor(fullScript.string());
                                }
                                ImGui::Separator();
                            }
                            if (meta.type == AssetType::Material) {
                                if (ImGui::MenuItem("🎨 Open in Material Editor")) {
                                    std::filesystem::path fullMat = contentRootPath / meta.sourcePath;
                                    OpenMaterialEditor(fullMat.string());
                                }
                                if (ImGui::MenuItem("🎯 Apply to Selected Actor")) {
                                    std::filesystem::path fullMat = contentRootPath / meta.sourcePath;
                                    MaterialAsset ma;
                                    if (LoadMaterialFile(fullMat.string(), ma)) {
                                        GameObject* sel = scene.GetSelected();
                                        if (sel) {
                                            sel->materialName = ma.name;
                                            sel->color = ma.baseColor;
                                            sel->metallic = ma.metallic;
                                            sel->roughness = ma.roughness;
                                            sel->normalStrength = ma.normalStrength;
                                            sel->specular = ma.specular;
                                            sel->emissiveColor = ma.emissiveColor;
                                            sel->emissiveIntensity = ma.emissiveIntensity;
                                            sel->shadingModel = ma.shadingModel;
                                            sel->blendMode = ma.blendMode;
                                            sel->twoSided = ma.twoSided;
                                            sel->castShadows = ma.castShadows;
                                            sel->receiveShadows = ma.receiveShadows;
                                            sel->baseColorTexture = ma.baseColorTexture;
                                            sel->normalTexture = ma.normalTexture;
                                            sel->roughnessTexture = ma.roughnessTexture;
                                            sel->metallicTexture = ma.metallicTexture;
                                            sel->aoTexture = ma.aoTexture;
                                            sel->emissionTexture = ma.emissionTexture;
                                            AddLog("LogMaterial", "Applied material '" + ma.name + "' to Actor '" + sel->name + "'", 2);
                                        }
                                    }
                                }
                                ImGui::Separator();
                            }

                            if (meta.type == AssetType::Mesh) {
                                if (ImGui::MenuItem("🧊 Spawn Mesh in Level")) {
                                    std::filesystem::path fullMesh = contentRootPath / meta.sourcePath;
                                    GameObject& newObj = scene.AddImportedMesh(fullMesh.string(), glm::vec3(0.0f, 0.5f, 0.0f));
                                    scene.selectedId = newObj.id;
                                    AddLog("LogMesh", "Spawned 3D Mesh in level: " + meta.objectName, 2);
                                }
                                ImGui::Separator();
                            }

                            if (meta.type == AssetType::Level) {
                                if (ImGui::MenuItem("🌐 Open Level in Viewport")) {
                                    std::filesystem::path fullLevel = contentRootPath / meta.sourcePath;
                                    OpenLevelFromPath(scene, fullLevel.string());
                                }
                                ImGui::Separator();
                            }

                            if (ImGui::MenuItem("🔍 View in Reference Viewer")) {
                                OpenReferenceViewer(meta.id);
                            }
                            if (ImGui::MenuItem("📋 Copy AssetID")) {
                                ImGui::SetClipboardText(meta.id.ToString().c_str());
                                AddLog("LogAsset", "Copied AssetID to clipboard: " + meta.id.ToString(), 0);
                            }
                            if (ImGui::MenuItem("📋 Copy Virtual Path")) {
                                ImGui::SetClipboardText(meta.virtualPath.c_str());
                                AddLog("LogAsset", "Copied Virtual Path to clipboard: " + meta.virtualPath, 0);
                            }
                            ImGui::Separator();
                            if (ImGui::MenuItem("❌ Delete Asset (Safe Check)")) {
                                auto refs = AssetRegistry::Get().GetReferences(meta.id);
                                if (!refs.empty()) {
                                    AddLog("LogAsset", "Cannot delete '" + meta.objectName + "': referenced by " + std::to_string(refs.size()) + " assets! Inspect in Reference Viewer.", 3);
                                    OpenReferenceViewer(meta.id);
                                } else {
                                    std::filesystem::path fullDisk = contentRootPath / meta.sourcePath;
                                    std::filesystem::remove(fullDisk, ec);
                                    std::filesystem::remove(fullDisk.string() + ".assetmeta", ec);
                                    AssetRegistry::Get().UnregisterAsset(meta.id);
                                    AddLog("LogAsset", "Deleted asset: " + meta.objectName, 1);
                                }
                            }
                            ImGui::EndPopup();
                        }

                        ImGui::PopID();
                        rowIndex++;
                    }

                    if (rowIndex == 0) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextDisabled("-");
                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextDisabled("(No assets found in this virtual directory. Right-click or use top buttons to create)");
                    }

                    ImGui::EndTable();
                }

                ImGui::EndChild();
            }
            else if (bottomDrawerTab == 1) {
                // 2. Output Log
                ImGui::BeginChild("OutputLogView", ImVec2(0, -30), true);
                for (const auto& log : logs) {
                    ImVec4 col = (log.level == 2) ? ImVec4(0.3f, 0.9f, 0.4f, 1.0f) :
                                 (log.level == 1) ? ImVec4(1.0f, 0.8f, 0.3f, 1.0f) :
                                 ImVec4(0.7f, 0.75f, 0.8f, 1.0f);
                    ImGui::TextColored(col, "[%s] %s", log.category.c_str(), log.message.c_str());
                }
                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                    ImGui::SetScrollHereY(1.0f);
                }
                ImGui::EndChild();

                // Console command input
                ImGui::TextDisabled("Cmd: ");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
                if (ImGui::InputText("##ConsoleInput", consoleInput, sizeof(consoleInput), ImGuiInputTextFlags_EnterReturnsTrue)) {
                    if (consoleInput[0] != '\0') {
                        AddLog("LogCommand", consoleInput, 0);
                        consoleInput[0] = '\0';
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear")) logs.clear();
            }
        }
    }
    ImGui::End();
}

void EngineUI::RenderGizmo(Scene& scene, OrbitCamera& camera, float viewportWidth, float viewportHeight) {
    if (scene.isPlayMode || !showGizmo || scene.selectedId == -1) return;

    GameObject* obj = scene.GetSelected();
    if (!obj || !obj->visible) return;

    ImGuiViewport* vp = ImGui::GetMainViewport();
    if (!vp || vp->Size.x <= 10.0f || vp->Size.y <= 10.0f) return;

    // Only allow ImGuizmo to handle mouse if not interacting with UI windows, menus, or popups (e.g. Color Picker)
    bool mouseOverUI = ImGui::GetIO().WantCaptureMouse;
    ImGuizmo::PushID(obj->id);
    ImGuizmo::Enable(!mouseOverUI || ImGuizmo::IsUsing());
    ImGuizmo::SetOrthographic(false);

    // The DirectX 12 3D scene renders to the center viewport (vpRect) matching UI_Ref.svg
    ViewportRect vpRect = GetViewportRect(vp->Size.x, vp->Size.y);
    ImGuizmo::SetRect(vp->Pos.x + vpRect.x, vp->Pos.y + vpRect.y, vpRect.width, vpRect.height);

    bool isLight = (obj->isLight || IsLightPrimitive(obj->type));

    // When a light is selected, obtain the light's actual world-space position from its transform/component.
    // If the light has a parent, GetWorldMatrix computes: WorldTransform = ParentWorldTransform * LocalTransform.
    // Never use mesh bounds center, actor bounds center, or bounding-box center for lights.
    glm::mat4 modelMatrix = scene.GetWorldMatrix(*obj);
    if (!isLight && gizmoUseCenter && !obj->mesh.vertices.empty()) {
        glm::vec3 worldCenter = scene.GetWorldCenter(*obj);
        modelMatrix[3] = glm::vec4(worldCenter, 1.0f);
    }

    // Task 2: bail out before ImGuizmo::Manipulate if modelMatrix is corrupted.
    // Selecting a broken object should never hang rendering — it should just
    // not show a gizmo for it until its data is fixed.
    {
        const float* mp = glm::value_ptr(modelMatrix);
        bool matrixValid = true;
        for (int i = 0; i < 16; ++i) {
            if (std::isnan(mp[i]) || !std::isfinite(mp[i])) { matrixValid = false; break; }
        }
        if (!matrixValid) {
            static int lastWarnedId = -1;
            if (lastWarnedId != obj->id) {
                std::cerr << "[RECOVERY] Object \"" << obj->name << "\" (id=" << obj->id
                          << ") has a corrupted transform/mesh — gizmo suppressed to avoid GPU hang.\n";
                AddLog("LogRecovery",
                    "Object \"" + obj->name + "\" has a corrupted transform or mesh and its gizmo was suppressed. Fix or delete it.", 2);
                lastWarnedId = obj->id;
            }
            ImGuizmo::PopID();
            return;
        }
    }
    glm::mat4 viewMatrix = camera.GetViewMatrix();
    glm::mat4 projMatrix = camera.GetProjectionMatrix(vpRect.width / vpRect.height);

    float snapValues[3];
    float* pSnap = nullptr;
    if (useSnap) {
        if (currentGizmoOperation == ImGuizmo::ROTATE) {
            snapValues[0] = snapRotation;
            pSnap = snapValues;
        } else if (currentGizmoOperation == ImGuizmo::SCALE) {
            snapValues[0] = snapValues[1] = snapValues[2] = snapScale;
            pSnap = snapValues;
        } else {
            snapValues[0] = snapTranslation.x;
            snapValues[1] = snapTranslation.y;
            snapValues[2] = snapTranslation.z;
            pSnap = snapValues;
        }
    }

    // Scale operation in ImGuizmo requires LOCAL mode
    ImGuizmo::MODE actualMode = (currentGizmoOperation == ImGuizmo::SCALE) 
        ? ImGuizmo::LOCAL 
        : (ImGuizmo::MODE)currentGizmoMode;

    bool manipulated = ImGuizmo::Manipulate(
        glm::value_ptr(viewMatrix),
        glm::value_ptr(projMatrix),
        (ImGuizmo::OPERATION)currentGizmoOperation,
        actualMode,
        glm::value_ptr(modelMatrix),
        nullptr,
        pSnap
    );

    static bool s_isGizmoUsing = false;
    if (ImGuizmo::IsUsing() || manipulated) {
        if (!s_isGizmoUsing) {
            s_isGizmoUsing = true;
            UndoManager::Get().RecordSnapshot(scene, "Transform Actor: " + obj->name);
        }
        obj->autoRotate = false; // Pause and disable auto-spinning when user is moving object

        // --- Task 1: Guard parent-inverse against degenerate/singular parent transforms ---
        // Converts World Space -> Local Space for child components/lights
        glm::mat4 localMatrix = modelMatrix;
        if (obj->parentId != -1) {
            glm::mat4 parentWorld = scene.GetWorldMatrix(obj->parentId);
            float parentDet = glm::determinant(parentWorld);
            if (std::abs(parentDet) > 1e-6f && !std::isnan(parentDet)) {
                localMatrix = glm::inverse(parentWorld) * modelMatrix;
            }
            // else: parent transform is degenerate — fall back to using modelMatrix
            // as-is (world space) rather than propagating Inf/NaN into localMatrix.
        }

        // Helper lambda: check a vec3 is safe (no NaN, no Inf)
        auto isVec3Valid = [](const glm::vec3& v) -> bool {
            return !std::isnan(v.x) && !std::isnan(v.y) && !std::isnan(v.z) &&
                    std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
        };

        if (currentGizmoOperation == ImGuizmo::TRANSLATE) {
            // Directly extract translation without Euler angle decomposition to prevent jitter
            glm::vec3 newPos;
            if (!isLight && gizmoUseCenter && !obj->mesh.vertices.empty()) {
                glm::vec3 minB(1e9f), maxB(-1e9f);
                for (const auto& v : obj->mesh.vertices) {
                    minB = glm::min(minB, v.pos);
                    maxB = glm::max(maxB, v.pos);
                }
                glm::vec3 localCenterOffset = (minB + maxB) * 0.5f;
                newPos = glm::vec3(localMatrix[3][0], localMatrix[3][1], localMatrix[3][2]) - localCenterOffset;
            } else {
                newPos = glm::vec3(localMatrix[3][0], localMatrix[3][1], localMatrix[3][2]);
            }
            // Task 2: only apply if the new position is finite/non-NaN
            if (isVec3Valid(newPos)) {
                obj->position = newPos;
            }
        } else if (currentGizmoOperation == ImGuizmo::SCALE) {
            // Task 2: guard scale extraction against NaN / non-finite column lengths
            glm::vec3 newScale(
                glm::length(glm::vec3(localMatrix[0])),
                glm::length(glm::vec3(localMatrix[1])),
                glm::length(glm::vec3(localMatrix[2]))
            );
            if (isVec3Valid(newScale)) {
                // Clamp to a small positive minimum so scale can never hit exactly 0
                // (which would make THIS object's own world matrix singular for future children/gizmo use)
                const float kMinScale = 1e-4f;
                obj->scale = glm::max(newScale, glm::vec3(kMinScale));
            }
            // else: ignore this frame's gizmo update rather than corrupting obj->scale.
        } else if (currentGizmoOperation == ImGuizmo::ROTATE) {
            float translation[3], rotation[3], scale[3];
            ImGuizmo::DecomposeMatrixToComponents(
                glm::value_ptr(localMatrix),
                translation,
                rotation,
                scale
            );
            glm::vec3 newRot(rotation[0], rotation[1], rotation[2]);
            if (isVec3Valid(newRot)) {
                obj->rotation = newRot;
            }
        } else {
            float translation[3], rotation[3], scale[3];
            ImGuizmo::DecomposeMatrixToComponents(
                glm::value_ptr(localMatrix),
                translation,
                rotation,
                scale
            );
            glm::vec3 newPos(translation[0], translation[1], translation[2]);
            glm::vec3 newRot(rotation[0], rotation[1], rotation[2]);
            glm::vec3 newSca(scale[0], scale[1], scale[2]);
            if (isVec3Valid(newPos)) obj->position = newPos;
            if (isVec3Valid(newRot)) obj->rotation = newRot;
            if (isVec3Valid(newSca)) {
                const float kMinScale = 1e-4f;
                obj->scale = glm::max(newSca, glm::vec3(kMinScale));
            }
        }

        if (isLight) {
            scene.SyncLightPositionsFromActors();
        }
    }

    if (!ImGuizmo::IsUsing()) {
        s_isGizmoUsing = false;
    }
    scene.isInteracting = ImGuizmo::IsUsing();
    ImGuizmo::PopID();
}

void EngineUI::RenderHelpModal() {
    ImGui::OpenPopup("Eunoia-Editor Guide");
    if (ImGui::BeginPopupModal("Eunoia-Editor Guide", &showHelpModal, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(0.12f, 0.68f, 1.00f, 1.0f), "Viewport Fly Mode & Camera Controls (Unreal Engine):");
        ImGui::BulletText("Hold RMB + W: Fly Forward");
        ImGui::BulletText("Hold RMB + S: Fly Backward");
        ImGui::BulletText("Hold RMB + A: Strafe Left");
        ImGui::BulletText("Hold RMB + D: Strafe Right");
        ImGui::BulletText("Hold RMB + E: Move Up");
        ImGui::BulletText("Hold RMB + Q: Move Down");
        ImGui::BulletText("Hold RMB + Mouse Movement: Look Around (360 FPS camera)");
        ImGui::BulletText("Hold RMB + Mouse Wheel: Increase / Decrease Camera Speed");
        ImGui::BulletText("Hold Shift while flying: Sprint / 2.5x Speed Boost");
        ImGui::BulletText("F Key: Focus Selected Actor in Viewport");
        ImGui::BulletText("Alt + LMB + Drag: Orbit Around Pivot / Selected Object");
        ImGui::BulletText("Alt + RMB + Drag: Dolly / Zoom");
        ImGui::BulletText("Alt + MMB + Drag: Pan / Track Viewport");
        ImGui::BulletText("F11 Key: Toggle Immersive Viewport Mode");
        ImGui::BulletText("G Key: Toggle Game View (Hide Grid & Gizmos)");
        ImGui::Separator();

        ImGui::TextColored(ImVec4(0.12f, 0.68f, 1.00f, 1.0f), "Transform Gizmos & Shortcuts:");
        ImGui::BulletText("W Key: Translate Tool");
        ImGui::BulletText("E Key: Rotate Tool");
        ImGui::BulletText("R Key: Scale Tool");
        ImGui::BulletText("Q Key: Toggle World / Local Space");
        ImGui::BulletText("Ctrl + D: Duplicate Actor");
        ImGui::BulletText("Delete Key: Delete Actor");
        ImGui::Separator();

        ImGui::TextColored(ImVec4(0.12f, 0.68f, 1.00f, 1.0f), "Editor Layout Overview (UI_Ref Blueprint):");
        ImGui::BulletText("Top Bar: Unified Menu Bar, Tool toggles, Simulation PIE controls");
        ImGui::BulletText("Outliner (Left): Actor hierarchy, visibility, duplicate/delete");
        ImGui::BulletText("Details (Right): Transform pills, Mobility, Static Mesh, Materials");
        ImGui::BulletText("Content Browser (Bottom): Quick-spawn asset shapes, Output Log");
        ImGui::Separator();

        if (ImGui::Button("Close Guide", ImVec2(120, 0))) {
            showHelpModal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void EngineUI::RenderReferenceViewer(Scene& scene) {
    ImGui::SetNextWindowSize(ImVec2(800, 520), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Asset Reference Viewer (dev.md Section 33)", &showReferenceViewer)) {
        ImGui::End();
        return;
    }

    const auto& allAssets = AssetRegistry::Get().GetAllAssets();
    if (allAssets.empty()) {
        ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "No assets currently registered in AssetRegistry.");
        ImGui::End();
        return;
    }

    // Header Asset Selector
    ImGui::TextColored(ImVec4(0.12f, 0.68f, 1.00f, 1.0f), "Select Asset to Inspect:");
    ImGui::SameLine();
    
    std::string currentAssetName = "None";
    const AssetMetadata* currentMeta = AssetRegistry::Get().FindById(refViewerSelectedAsset);
    if (currentMeta) {
        currentAssetName = std::string("[") + AssetTypeToString(currentMeta->type) + "] " + currentMeta->objectName + " (" + currentMeta->virtualPath + ")";
    }

    if (ImGui::BeginCombo("##RefViewerSelect", currentAssetName.c_str())) {
        for (const auto& pair : allAssets) {
            const AssetMetadata& m = pair.second;
            bool isSel = (m.id == refViewerSelectedAsset);
            std::string label = std::string("[") + AssetTypeToString(m.type) + "] " + m.objectName + " (" + m.virtualPath + ")";
            if (ImGui::Selectable(label.c_str(), isSel)) {
                refViewerSelectedAsset = m.id;
            }
            if (isSel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if (!currentMeta) {
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Please select an asset above to view its dependency and reference graph.");
        ImGui::End();
        return;
    }

    ImGui::Separator();

    // Asset Info Card
    ImGui::BeginChild("AssetInfoCard", ImVec2(0, 110), true);
    ImGui::Columns(2, "AssetInfoCols", false);
    ImGui::SetColumnWidth(0, 380);

    ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Asset Details:");
    ImGui::Text("Object Name:  %s", currentMeta->objectName.c_str());
    ImGui::Text("Type:         %s", AssetTypeToString(currentMeta->type));
    ImGui::Text("Virtual Path: %s", currentMeta->virtualPath.c_str());

    ImGui::NextColumn();
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Identifiers & Storage:");
    ImGui::Text("AssetID:      %s", currentMeta->id.ToString().c_str());
    ImGui::SameLine();
    if (ImGui::SmallButton("Copy ID##Ref")) {
        ImGui::SetClipboardText(currentMeta->id.ToString().c_str());
        AddLog("LogAsset", "Copied AssetID to clipboard: " + currentMeta->id.ToString(), 0);
    }
    ImGui::Text("Source File:  %s", currentMeta->sourcePath.c_str());
    ImGui::Text("Cooked Path:  %s", currentMeta->cookedPath.empty() ? "(Not cooked yet)" : currentMeta->cookedPath.c_str());

    ImGui::Columns(1);
    ImGui::EndChild();

    ImGui::Spacing();

    // Split Graph: Left = Dependencies (Assets this uses), Right = References (Assets/Actors using this)
    float colWidth = (ImGui::GetContentRegionAvail().x - 16.0f) * 0.5f;

    ImGui::BeginChild("DependenciesPanel", ImVec2(colWidth, 260), true);
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "Dependencies (What this asset uses):");
    ImGui::TextDisabled("Direct outbound references required by %s", currentMeta->objectName.c_str());
    ImGui::Separator();

    if (currentMeta->dependencies.empty()) {
        ImGui::TextDisabled("No outbound dependencies recorded.");
    } else {
        ImGui::BeginTable("DepsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 70);
        ImGui::TableSetupColumn("Asset", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 60);
        ImGui::TableHeadersRow();

        for (const auto& depId : currentMeta->dependencies) {
            const AssetMetadata* depMeta = AssetRegistry::Get().FindById(depId);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            if (depMeta) {
                ImGui::Text("%s", AssetTypeToString(depMeta->type));
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", depMeta->objectName.c_str());
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s\n%s", depMeta->virtualPath.c_str(), depId.ToString().c_str());
            } else {
                ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Missing");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "%s", depId.ToString().c_str());
            }

            ImGui::TableSetColumnIndex(2);
            std::string btnLabel = "View##" + depId.ToString();
            if (ImGui::SmallButton(btnLabel.c_str())) {
                refViewerSelectedAsset = depId;
            }
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("ReferencesPanel", ImVec2(colWidth, 260), true);
    ImGui::TextColored(ImVec4(0.3f, 0.85f, 1.0f, 1.0f), "References (What uses this asset):");
    ImGui::TextDisabled("Assets & World Actors referencing this asset");
    ImGui::Separator();

    // 1. Other Assets referencing this
    const auto& refs = currentMeta->references;
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.9f, 1.0f), "Asset References (%zu):", refs.size());
    if (refs.empty()) {
        ImGui::TextDisabled("No other registered assets depend on this.");
    } else {
        for (const auto& refId : refs) {
            const AssetMetadata* rMeta = AssetRegistry::Get().FindById(refId);
            if (rMeta) {
                ImGui::BulletText("[%s] %s", AssetTypeToString(rMeta->type), rMeta->objectName.c_str());
                ImGui::SameLine();
                std::string btnLabel = "Inspect##" + refId.ToString();
                if (ImGui::SmallButton(btnLabel.c_str())) {
                    refViewerSelectedAsset = refId;
                }
            } else {
                ImGui::BulletText("Unknown (%s)", refId.ToString().c_str());
            }
        }
    }

    ImGui::Spacing();
    ImGui::Separator();

    // 2. Active Level Actors referencing this
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.9f, 1.0f), "Active Level Actors referencing this:");
    int sceneRefCount = 0;
    for (const auto& obj : scene.objects) {
        bool match = false;
        if (currentMeta->type == AssetType::Material) {
            if (obj.materialName == currentMeta->objectName || obj.materialName == currentMeta->virtualPath) {
                match = true;
            }
        } else if (currentMeta->type == AssetType::Texture) {
            if (obj.baseColorTexture == currentMeta->objectName || obj.baseColorTexture == currentMeta->sourcePath ||
                obj.normalTexture == currentMeta->objectName || obj.roughnessTexture == currentMeta->objectName ||
                obj.metallicTexture == currentMeta->objectName || obj.aoTexture == currentMeta->objectName ||
                obj.emissionTexture == currentMeta->objectName) {
                match = true;
            }
        }
        if (match) {
            sceneRefCount++;
            ImGui::BulletText("Actor '%s' (ID: %d)", obj.name.c_str(), obj.id);
            ImGui::SameLine();
            std::string btn = "Select##Actor" + std::to_string(obj.id);
            if (ImGui::SmallButton(btn.c_str())) {
                scene.selectedId = obj.id;
                AddLog("LogRefViewer", "Selected Actor '" + obj.name + "' from Reference Viewer", 0);
            }
        }
    }
    if (sceneRefCount == 0) {
        ImGui::TextDisabled("No active level actors currently bound.");
    }

    ImGui::EndChild();

    ImGui::Spacing();
    if (ImGui::Button("Close Reference Viewer", ImVec2(180, 26))) {
        showReferenceViewer = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Refresh Graph", ImVec2(130, 26))) {
        AssetRegistry::Get().ScanDirectory(contentRootPath);
        AddLog("LogRefViewer", "Refreshed Asset Registry dependency graph", 0);
    }

    ImGui::End();
}

void EngineUI::RenderCookModal() {
    ImGui::SetNextWindowSize(ImVec2(650, 420), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Asset Cooking Pipeline (dev.md Section 24, 25)", &showCookModal)) {
        ImGui::End();
        return;
    }

    ImGui::TextColored(ImVec4(0.12f, 0.68f, 1.00f, 1.0f), "Project Asset Cooking & Packaging");
    ImGui::TextWrapped("Cooks source assets into runtime-optimized format and writes the Cooked Asset Registry (CookedAssetRegistry.json).");
    ImGui::Separator();

    static char outputDirBuf[128] = "Cooked/Content";
    ImGui::InputText("Target Directory", outputDirBuf, sizeof(outputDirBuf));

    static int targetPlatform = 0;
    const char* platforms[] = { "Windows (DirectX 12)", "Universal (SPIR-V / HLSL)" };
    ImGui::Combo("Platform Target", &targetPlatform, platforms, IM_ARRAYSIZE(platforms));

    ImGui::Spacing();
    if (ImGui::Button("🚀 Start Cooking Now", ImVec2(180, 30))) {
        cookLog += "\n=== Starting Asset Cooking Pipeline ===\n";
        cookLog += "Target Platform: " + std::string(platforms[targetPlatform]) + "\n";
        cookLog += "Destination: " + std::string(outputDirBuf) + "\n";

        bool ok = AssetManager::Get().CookProject(outputDirBuf, cookLog);
        if (ok) {
            cookLog += ">>> SUCCESS: All project assets cooked successfully!\n";
            cookLog += ">>> Cooked Asset Registry saved to: " + std::string(outputDirBuf) + "/CookedAssetRegistry.json\n";
            AddLog("LogCook", "Project cooking finished successfully at " + std::string(outputDirBuf), 2);
        } else {
            cookLog += ">>> ERROR: Asset cooking encountered errors. Check output log.\n";
            AddLog("LogCook", "Asset cooking failed! Check log.", 1);
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear Cook Log", ImVec2(120, 30))) {
        cookLog = "";
    }
    ImGui::SameLine();
    if (ImGui::Button("Close", ImVec2(90, 30))) {
        showCookModal = false;
    }

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Cooking Output Log:");
    ImGui::BeginChild("CookLogRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::TextUnformatted(cookLog.c_str());
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    ImGui::End();
}

// ============================================================================
// Play Mode / Editor Mode Management (dev.md Section 30-36)
// ============================================================================

void EngineUI::EnterPlayMode(Scene& scene) {
    if (scene.isPlayMode) return;
    scene.StartPlayMode();
    AddLog("LogPlayLevel", "PIE: Play Mode Started. Running Behaviours...", 0);
}

void EngineUI::ExitPlayMode(Scene& scene) {
    if (!scene.isPlayMode) return;
    scene.StopPlayMode();
    AddLog("LogPlayLevel", "PIE: Play Mode Stopped (DELETE). Restored Editor Scene.", 0);
}

// ============================================================================
// In-Editor Code Editor (dev.md & User Request: double click behaviour script)
// ============================================================================

void EngineUI::OpenScriptInCodeEditor(const std::string& path) {
    activeCodeEditorPath = path;
    activeCodeEditorFilename = std::filesystem::path(path).filename().string();
    activeCodeEditorContent.clear();

    std::ifstream file(path);
    if (file.is_open()) {
        std::stringstream ss;
        ss << file.rdbuf();
        activeCodeEditorContent = ss.str();
    }
    showCodeEditor = true;
    codeEditorDirty = false;
    AddLog("LogContent", "Opened Behaviour script in Code Editor: " + activeCodeEditorFilename, 0);
}

void EngineUI::RenderCodeEditor() {
    if (!showCodeEditor) return;

    std::string title = "💻 Code Editor — " + activeCodeEditorFilename + (codeEditorDirty ? "*" : "") + "###CodeEditorWin";
    ImGui::SetNextWindowSize(ImVec2(740, 540), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(title.c_str(), &showCodeEditor, ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::MenuItem("💾 Save File", "Ctrl+S")) {
                std::ofstream file(activeCodeEditorPath);
                if (file.is_open()) {
                    file << activeCodeEditorContent;
                    codeEditorDirty = false;
                    AddLog("LogContent", "Saved file: " + activeCodeEditorFilename, 2);
                }
            }
            if (ImGui::MenuItem("🚀 Open in External IDE")) {
                ShellExecuteA(NULL, "open", activeCodeEditorPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
                AddLog("LogContent", "Launched external editor for: " + activeCodeEditorFilename, 0);
            }
            ImGui::EndMenuBar();
        }

        // Action Toolbar
        if (ImGui::Button("💾 Save File")) {
            std::ofstream file(activeCodeEditorPath);
            if (file.is_open()) {
                file << activeCodeEditorContent;
                codeEditorDirty = false;
                AddLog("LogContent", "Saved file: " + activeCodeEditorFilename, 2);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("🚀 Open in External IDE (VS Code / Visual Studio)")) {
            ShellExecuteA(NULL, "open", activeCodeEditorPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
            AddLog("LogContent", "Launched external editor for: " + activeCodeEditorFilename, 0);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("| %s", activeCodeEditorPath.c_str());

        ImGui::Separator();

        // Multiline text buffer
        static std::vector<char> buffer;
        if (buffer.size() < activeCodeEditorContent.size() + 65536) {
            buffer.resize(activeCodeEditorContent.size() + 65536);
            memcpy(buffer.data(), activeCodeEditorContent.c_str(), activeCodeEditorContent.size() + 1);
        }

        ImVec2 avail = ImGui::GetContentRegionAvail();
        if (ImGui::InputTextMultiline("##CodeEditorText", buffer.data(), buffer.size(), avail,
            ImGuiInputTextFlags_AllowTabInput)) {
            activeCodeEditorContent = buffer.data();
            codeEditorDirty = true;
        }

        // Ctrl+S hotkey
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
            ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
            std::ofstream file(activeCodeEditorPath);
            if (file.is_open()) {
                file << activeCodeEditorContent;
                codeEditorDirty = false;
                AddLog("LogContent", "Saved file: " + activeCodeEditorFilename, 2);
            }
        }
    }
    ImGui::End();
}

// ============================================================================
// Details Panel Behaviours Section (dev.md Section 6, 7, 8, 9, 10, 11, 12, 13)
// ============================================================================

void EngineUI::RenderBehavioursSection(Scene& scene, GameObject* obj) {
    if (!obj) return;

    ImGui::Spacing();
    ImGui::Separator();
    char headerLabel[128];
    snprintf(headerLabel, sizeof(headerLabel), "🧩 Behaviours (%zu)###BehavioursHeader", obj->behaviours.size());
    if (ImGui::CollapsingHeader(headerLabel, ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();

        // Button to add behaviour
        if (ImGui::Button("+ Add Behaviour", ImVec2(ImGui::GetContentRegionAvail().x, 26.0f))) {
            showAddBehaviourPopup = true;
            behaviourSearchBuf[0] = '\0';
        }

        if (showAddBehaviourPopup) {
            ImGui::OpenPopup("Add Behaviour##Modal");
            showAddBehaviourPopup = false;
        }

        if (ImGui::BeginPopupModal("Add Behaviour##Modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0.12f, 0.68f, 1.00f, 1.0f), "Select Behaviour to Attach:");
            ImGui::SetNextItemWidth(300.0f);
            ImGui::InputTextWithHint("##BehSearch", "🔍 Search Behaviours...", behaviourSearchBuf, sizeof(behaviourSearchBuf));
            ImGui::Separator();

            std::string searchLower = behaviourSearchBuf;
            std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

            const auto& allBehaviors = BehaviourRegistry::Get().GetAll();
            int matchCount = 0;
            for (const auto& pair : allBehaviors) {
                const auto& info = pair.second;
                std::string nameLower = info.displayName;
                std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                std::string classLower = info.className;
                std::transform(classLower.begin(), classLower.end(), classLower.begin(), ::tolower);

                if (!searchLower.empty() && nameLower.find(searchLower) == std::string::npos && classLower.find(searchLower) == std::string::npos) {
                    continue;
                }

                matchCount++;
                std::string itemText = "🧩 " + info.displayName + " (" + info.className + ")";
                if (ImGui::Selectable(itemText.c_str(), false)) {
                    UndoManager::Get().RecordSnapshot(scene, "Add Behaviour: " + info.className);
                    auto newB = BehaviourRegistry::Get().Create(info.className);
                    if (newB) {
                        newB->SetScene(&scene);
                        obj->AddBehaviour(std::move(newB));
                        obj->behaviours.back()->ResolveReferences(scene);
                        AddLog("LogBehaviour", "Attached " + info.displayName + " to " + obj->name, 2);
                    }
                    ImGui::CloseCurrentPopup();
                }
                if (!info.description.empty() && ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", info.description.c_str());
                }
            }

            if (matchCount == 0) {
                ImGui::TextDisabled("No behaviours match search query.");
            }

            ImGui::Separator();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Render each attached behaviour
        int removeIdx = -1;
        int moveUpIdx = -1;
        int moveDownIdx = -1;

        for (size_t i = 0; i < obj->behaviours.size(); ++i) {
            auto& b = obj->behaviours[i];
            if (!b) continue;

            ImGui::PushID((int)i);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
            ImGui::BeginChild("BehaviourCard", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);

            // Header line: Reorder, Class Title, Enable checkbox, Remove button
            if (i > 0) {
                if (ImGui::SmallButton("▲")) moveUpIdx = (int)i;
                ImGui::SameLine();
            }
            if (i + 1 < obj->behaviours.size()) {
                if (ImGui::SmallButton("▼")) moveDownIdx = (int)i;
                ImGui::SameLine();
            }

            bool isEnabled = b->IsEnabled();
            if (ImGui::Checkbox("##Enabled", &isEnabled)) {
                UndoManager::Get().RecordSnapshot(scene, "Toggle Behaviour Enabled");
                b->SetEnabled(isEnabled);
            }
            ImGui::SameLine();

            ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "%s", b->GetDisplayName().c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("(%s)", b->GetClassName().c_str());

            // Right-aligned Remove button
            float availW = ImGui::GetContentRegionAvail().x;
            if (availW > 70.0f) {
                ImGui::SameLine(ImGui::GetWindowWidth() - 75.0f);
                if (ImGui::SmallButton("🗑 Remove")) {
                    removeIdx = (int)i;
                }
            }

            ImGui::Separator();

            // Properties list
            auto& props = b->GetProperties();
            for (size_t pi = 0; pi < props.size(); ++pi) {
                auto& prop = props[pi];
                ImGui::PushID((int)pi);

                switch (prop.type) {
                    case BehaviourPropertyType::Bool: {
                        if (prop.dataPtr) {
                            bool* val = reinterpret_cast<bool*>(prop.dataPtr);
                            ImGui::Checkbox(prop.displayName.c_str(), val);
                        }
                        break;
                    }
                    case BehaviourPropertyType::Int: {
                        if (prop.dataPtr) {
                            int* val = reinterpret_cast<int*>(prop.dataPtr);
                            if (prop.hasRange) {
                                ImGui::SliderInt(prop.displayName.c_str(), val, (int)prop.minVal, (int)prop.maxVal);
                            } else {
                                ImGui::DragInt(prop.displayName.c_str(), val);
                            }
                        }
                        break;
                    }
                    case BehaviourPropertyType::Float: {
                        if (prop.dataPtr) {
                            float* val = reinterpret_cast<float*>(prop.dataPtr);
                            if (prop.hasRange) {
                                ImGui::SliderFloat(prop.displayName.c_str(), val, prop.minVal, prop.maxVal);
                            } else {
                                ImGui::DragFloat(prop.displayName.c_str(), val, 0.1f);
                            }
                        }
                        break;
                    }
                    case BehaviourPropertyType::Double: {
                        if (prop.dataPtr) {
                            double* val = reinterpret_cast<double*>(prop.dataPtr);
                            float tempF = (float)*val;
                            if (ImGui::DragFloat(prop.displayName.c_str(), &tempF, 0.1f)) {
                                *val = (double)tempF;
                            }
                        }
                        break;
                    }
                    case BehaviourPropertyType::String: {
                        if (prop.dataPtr) {
                            std::string* val = reinterpret_cast<std::string*>(prop.dataPtr);
                            char strBuf[256] = {};
                            strncpy(strBuf, val->c_str(), sizeof(strBuf) - 1);
                            if (ImGui::InputText(prop.displayName.c_str(), strBuf, sizeof(strBuf))) {
                                *val = strBuf;
                            }
                        }
                        break;
                    }
                    case BehaviourPropertyType::Vec2: {
                        if (prop.dataPtr) {
                            glm::vec2* val = reinterpret_cast<glm::vec2*>(prop.dataPtr);
                            ImGui::DragFloat2(prop.displayName.c_str(), &val->x, 0.1f);
                        }
                        break;
                    }
                    case BehaviourPropertyType::Vec3: {
                        if (prop.dataPtr) {
                            glm::vec3* val = reinterpret_cast<glm::vec3*>(prop.dataPtr);
                            ImGui::DragFloat3(prop.displayName.c_str(), &val->x, 0.1f);
                        }
                        break;
                    }
                    case BehaviourPropertyType::Vec4: {
                        if (prop.dataPtr) {
                            glm::vec4* val = reinterpret_cast<glm::vec4*>(prop.dataPtr);
                            ImGui::DragFloat4(prop.displayName.c_str(), &val->x, 0.1f);
                        }
                        break;
                    }
                    case BehaviourPropertyType::Color3: {
                        if (prop.dataPtr) {
                            glm::vec3* val = reinterpret_cast<glm::vec3*>(prop.dataPtr);
                            ImGui::ColorEdit3(prop.displayName.c_str(), &val->x);
                        }
                        break;
                    }
                    case BehaviourPropertyType::ObjectRef: {
                        // Object Reference with Typed Dropdown and Drag & Drop (dev.md Section 9, 10, 11, 12, 13)
                        ImGui::Text("%s", prop.displayName.c_str());

                        std::string preview = "None (Select " + std::string(ObjectRefTypeToString(prop.refType)) + ")";
                        if (prop.targetId != -1) {
                            GameObject* targetObj = scene.FindObject(prop.targetId);
                            if (targetObj) {
                                preview = targetObj->name;
                            } else {
                                preview = "⚠ Missing Reference (ID: " + std::to_string(prop.targetId) + ")";
                            }
                        }

                        if (prop.isMissing) {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.35f, 0.35f, 1.0f));
                        }

                        if (ImGui::BeginCombo("##ObjRefCombo", preview.c_str())) {
                            if (ImGui::Selectable("(None)", prop.targetId == -1)) {
                                UndoManager::Get().RecordSnapshot(scene, "Clear Reference: " + prop.name);
                                prop.targetId = -1;
                                b->ResolveReferences(scene);
                            }
                            ImGui::Separator();

                            for (const auto& sceneObj : scene.objects) {
                                // Filter by requested reference type (dev.md Section 10)
                                if (prop.refType == ObjectRefType::Light && !sceneObj.isLight) continue;
                                if (prop.refType == ObjectRefType::Mesh && sceneObj.mesh.indices.empty()) continue;

                                bool isSelected = (prop.targetId == sceneObj.id);
                                std::string itemLabel = sceneObj.name + " (ID: " + std::to_string(sceneObj.id) + ")";
                                if (ImGui::Selectable(itemLabel.c_str(), isSelected)) {
                                    UndoManager::Get().RecordSnapshot(scene, "Assign Reference: " + prop.name);
                                    prop.targetId = sceneObj.id;
                                    b->ResolveReferences(scene);
                                }
                            }
                            ImGui::EndCombo();
                        }

                        if (prop.isMissing) {
                            ImGui::PopStyleColor();
                        }

                        // Drag & Drop Target from Hierarchy (dev.md Section 12)
                        if (ImGui::BeginDragDropTarget()) {
                            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("OUTLINER_ACTOR")) {
                                int draggedId = *(const int*)payload->Data;
                                GameObject* draggedObj = scene.FindObject(draggedId);
                                if (draggedObj) {
                                    bool compatible = true;
                                    if (prop.refType == ObjectRefType::Light && !draggedObj->isLight) compatible = false;
                                    if (prop.refType == ObjectRefType::Mesh && draggedObj->mesh.indices.empty()) compatible = false;

                                    if (compatible) {
                                        UndoManager::Get().RecordSnapshot(scene, "Drop Reference: " + prop.name);
                                        prop.targetId = draggedId;
                                        b->ResolveReferences(scene);
                                        AddLog("LogBehaviour", "Assigned " + draggedObj->name + " to " + prop.displayName, 2);
                                    } else {
                                        AddLog("LogBehaviour", "Cannot assign " + draggedObj->name + " — incompatible type (" + ObjectRefTypeToString(prop.refType) + " expected).", 1);
                                    }
                                }
                            }
                            ImGui::EndDragDropTarget();
                        }

                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Select %s or drag & drop actor from Outliner onto this field.", ObjectRefTypeToString(prop.refType));
                        }
                        break;
                    }
                }

                ImGui::PopID();
            }

            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImGui::PopID();
        }

        if (removeIdx >= 0) {
            UndoManager::Get().RecordSnapshot(scene, "Remove Behaviour");
            obj->RemoveBehaviour((size_t)removeIdx);
        } else if (moveUpIdx > 0) {
            UndoManager::Get().RecordSnapshot(scene, "Reorder Behaviour");
            obj->ReorderBehaviour((size_t)moveUpIdx, (size_t)(moveUpIdx - 1));
        } else if (moveDownIdx >= 0 && moveDownIdx + 1 < (int)obj->behaviours.size()) {
            UndoManager::Get().RecordSnapshot(scene, "Reorder Behaviour");
            obj->ReorderBehaviour((size_t)moveDownIdx, (size_t)(moveDownIdx + 1));
        }
    }
}


