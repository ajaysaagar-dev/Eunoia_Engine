#pragma once
#include "Scene.h"
#include "Camera.h"
#include "AssetSystem.h"
#include "SceneSerializer.h"
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_set>

struct EngineLogEntry {
    std::string category;
    std::string message;
    int level; // 0=Info, 1=Warning, 2=Success, 3=Error
};

struct TextureAssetEntry {
    std::string filename;  // e.g. "grass_ground_diff_2k.png"
    std::string relPath;   // e.g. "Materials/textures/grass_ground_diff_2k.png"
    std::string relFolder; // e.g. "Materials/textures"
    std::string fullPath;  // absolute path
    AssetID assetId;       // Stable AssetID
    std::string virtualPath; // e.g. "/Game/Materials/textures/grass_ground_diff_2k"
};

struct MaterialAsset {
    AssetID assetId;
    std::string virtualPath = "";
    std::string name = "M_NewMaterial";
    std::string filePath = "";

    // Surface Textures with stable AssetIDs (dev.md Section 21)
    std::string baseColorTexture = "";
    AssetID baseColorAssetId;
    std::string normalTexture = "";
    AssetID normalAssetId;
    std::string roughnessTexture = "";
    AssetID roughnessAssetId;
    std::string metallicTexture = "";
    AssetID metallicAssetId;
    std::string aoTexture = "";
    AssetID aoAssetId;
    std::string emissionTexture = "";
    AssetID emissionAssetId;

    // Values (from dev.md)
    glm::vec3 baseColor{0.8f, 0.8f, 0.8f};
    float metallic = 0.0f;
    float roughness = 0.5f;
    float normalStrength = 1.0f;
    float specular = 0.5f;
    glm::vec3 emissiveColor{0.0f, 0.0f, 0.0f};
    float emissiveIntensity = 0.0f; // Emission Power

    // Settings (from dev.md)
    int shadingModel = 0; // 0=PBR (Default Lit), 1=Unlit, 2=Subsurface
    int blendMode = 0;    // 0=Opaque, 1=Masked, 2=Translucent
    bool twoSided = false;
    bool castShadows = true;
    bool receiveShadows = true;
};

class EngineUI {
public:
    // Window visibility flags (UE5 panels)
    bool showOutliner = true;
    bool showDetails = true;
    bool showBottomDrawer = true;
    bool showViewportOverlay = true;
    bool showHelpModal = false;
    bool showUndoHistory = false;

    // Bottom drawer active tab: 0=Content Browser, 1=Output Log
    int bottomDrawerTab = 0;
    bool bottomDrawerOpen = true;

    // Content Browser project root & navigation
    std::filesystem::path contentRootPath;
    std::filesystem::path currentContentPath;
    char contentBrowserSearch[64] = "";
    std::string selectedContentItem = "";

    // Folder & Material creation modal/state
    bool showNewFolderPopup = false;
    char newFolderNameBuf[64] = "NewFolder";

    bool showNewMaterialPopup = false;
    char newMaterialNameBuf[64] = "M_NewMaterial";

    // Material Editor Window
    bool showMaterialEditor = false;
    MaterialAsset activeMaterial;
    bool materialDirty = false;
    bool liveMaterialViewportSync = true;
    float matEditorLightAngle = 45.0f;

    // Gizmos configuration
    bool showGizmo = true;
    int currentGizmoOperation = 7; // ImGuizmo::TRANSLATE
    int currentGizmoMode = 1;      // ImGuizmo::WORLD
    bool gizmoUseCenter = false;   // false = Pivot (at object's pivot point, default), true = Center (mesh bounding-box center)
    bool useSnap = false;
    glm::vec3 snapTranslation{0.5f, 0.5f, 0.5f};
    float snapRotation = 15.0f; // degrees
    float snapScale = 0.25f;

    // UI layout dimensions (scaled for friendly visibility and comfort)
    float uiMargin = 0.0f;
    float uiGap = 0.0f;
    float topBarHeight = 62.0f;
    float leftSidebarWidth = 280.0f;
    float rightSidebarWidth = 320.0f;
    float bottomDockHeight = 260.0f;

    struct ViewportRect {
        float x, y, width, height;
    };

    ViewportRect GetViewportRect(float windowWidth, float windowHeight) const {
        if (isImmersiveMode) {
            return { 0.0f, 0.0f, windowWidth, windowHeight };
        }
        float topH = topBarHeight;
        float bottomH = showBottomDrawer ? (bottomDrawerOpen ? bottomDockHeight : 38.0f) : 0.0f;
        float leftW = showOutliner ? leftSidebarWidth : 0.0f;
        float rightW = showDetails ? rightSidebarWidth : 0.0f;

        float vpX = leftW;
        float vpY = topH;
        float vpW = windowWidth - leftW - rightW;
        float vpH = windowHeight - topH - bottomH;

        if (vpW < 10.0f) vpW = 10.0f;
        if (vpH < 10.0f) vpH = 10.0f;

        return { vpX, vpY, vpW, vpH };
    }

    // Viewport display mode
    int viewMode = 0; // 0=Lit, 1=Wireframe, 2=Unlit
    float cameraSpeed = 4.0f;
    bool lockAspectScale = false;

    // Outliner search filter
    char outlinerFilter[64] = "";
    char consoleInput[128] = "";

    std::vector<EngineLogEntry> logs;

    // Game View (G key) & Immersive Viewport (F11 key)
    bool isGameView = false;
    bool prevShowGrid = true;
    bool prevShowGizmo = true;

    bool isImmersiveMode = false;
    bool prevShowOutliner = true;
    bool prevShowDetails = true;
    bool prevShowBottomDrawer = true;

    // Level File Management (In whole project "Level" is used instead of "Scene")
    std::string currentLevelFilePath = "";
    bool levelUnsaved = false;
    std::string& currentSceneFilePath = currentLevelFilePath;
    bool& sceneUnsaved = levelUnsaved;

    void ToggleGameView(Scene& scene);
    void ToggleImmersiveMode();

    // Level Save/Load
    bool SaveLevel(Scene& level);                              // Save to current file (or Save As if new)
    bool SaveLevelAs(Scene& level);                            // Save to new file via dialog
    bool OpenLevel(Scene& level);                              // Open level via file dialog
    bool OpenLevelFromPath(Scene& level, const std::string& filePath); // Open level directly from disk path

    // Scene Save/Load aliases
    bool SaveScene(Scene& scene) { return SaveLevel(scene); }
    bool SaveSceneAs(Scene& scene) { return SaveLevelAs(scene); }
    bool OpenScene(Scene& scene) { return OpenLevel(scene); }

    static std::string ShowSaveFileDialog();                   // Win32 save level dialog
    static std::string ShowOpenFileDialog();                   // Win32 open level dialog
    static std::string ShowOpenMeshDialog();                   // Win32 open mesh dialog (.obj, .gltf, .glb)
    static std::string ShowSaveMeshLocationDialog(const std::string& defaultFilename, const std::string& initialDir);

    // Import Mesh with Save Location Prompt
    void ImportMeshWithSavePrompt(Scene& level);

    // OS File Drag & Drop (models from system to content browser)
    void HandleFileDrop(const char** paths, int count);

    // Viewport Drag & Drop Target (3D objects from content browser to viewport)
    void RenderViewportDropTarget(Scene& level, OrbitCamera& camera);
    glm::vec3 CalculateDropSpawnPosition(OrbitCamera& camera, float mouseX, float mouseY, float screenW, float screenH);

    EngineUI();

    void AddLog(const std::string& category, const std::string& message, int level = 0);
    void SetupTheme();
    void Render(Scene& scene, OrbitCamera& camera, float fps, float frameTimeMs, uint32_t vertexCount, uint32_t indexCount, bool& outShouldExit);
    void RenderGizmo(Scene& scene, OrbitCamera& camera, float viewportWidth, float viewportHeight);

    // Reference Viewer & Asset Cooking (dev.md Section 24, 33)
    bool showReferenceViewer = false;
    AssetID refViewerSelectedAsset;
    bool showCookModal = false;
    std::string cookLog = "";
    std::string currentVirtualDir = "/Game";

    void OpenReferenceViewer(const AssetID& id = AssetID::Null());
    void OpenCookModal();

    // Material file I/O & Textures
    bool LoadMaterialFile(const std::string& path, MaterialAsset& outMat);
    bool SaveMaterialFile(const std::string& path, const MaterialAsset& mat);
    void OpenMaterialEditor(const std::string& path);
    std::vector<TextureAssetEntry> ScanProjectTextures();
    bool DrawTextureSlot(const char* label, std::string& textureSlotValue, AssetID& textureSlotAssetId, const std::vector<TextureAssetEntry>& availableTextures);

    // Behaviours & Play Mode (dev.md)
    bool showAddBehaviourPopup = false;
    char behaviourSearchBuf[64] = "";
    bool showNewBehaviourPopup = false;
    char newBehaviourNameBuf[64] = "PlayerController";

    // In-Editor Code Editor (dev.md & User Request)
    bool showCodeEditor = false;
    std::string activeCodeEditorPath = "";
    std::string activeCodeEditorFilename = "";
    std::string activeCodeEditorContent = "";
    bool codeEditorDirty = false;

    void OpenScriptInCodeEditor(const std::string& path);
    void RenderCodeEditor();
    void RenderBehavioursSection(Scene& scene, GameObject* obj);
    void EnterPlayMode(Scene& scene);
    void ExitPlayMode(Scene& scene);

    uint64_t lightIconGpuHandle = 0;

private:
    void RenderTopMenuBar(Scene& scene, OrbitCamera& camera, bool& outShouldExit);
    void RenderViewportOverlay(Scene& scene, OrbitCamera& camera, float fps, float frameTimeMs);
    void RenderOutliner(Scene& scene);
    void DrawOutlinerNode(GameObject& obj, Scene& scene, std::unordered_set<int>& visitedIds, int depth = 0);
    void RenderDetails(Scene& scene);
    void RenderContentBrowser(Scene& scene);
    void RenderMaterialEditor(Scene& scene);
    void RenderReferenceViewer(Scene& scene);
    void RenderCookModal();
    void RenderHelpModal();

    // Custom UE5-style UI widgets
    bool DrawTransformPill(const char* label, float& value, const glm::vec4& color, float resetValue = 0.0f, float speed = 0.05f);

    int m_pendingReparentChild = -1;
    int m_pendingReparentParent = -1;
};
