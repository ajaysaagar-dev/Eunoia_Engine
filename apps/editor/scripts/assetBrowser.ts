import EngineRegistry from "../../../engine/registry.plugins";
import {
    DEFAULT_PROJECT_PATH,
    EnsureProjectDirectory,
    ReadProjectDirectoryFiles,
    CreateProjectFolder,
    RenameProjectItem,
    MoveProjectItem,
    DeleteProjectItem,
    type ScannedProjectItem
} from "../../../plugins/file-system.plugins";
import { CreateNewSceneFile, SaveActiveScene, LoadSceneFromFile } from "../../../plugins/scene.plugins";
import { updateHierarchyUI } from "./hierarchy";

export interface AssetFile {
    id: number; // Permanent numeric resource reference ID
    name: string;
    displayName: string;
    type: "mesh" | "material" | "texture" | "script" | "scene" | "folder" | "other";
    icon: string;
    path: string;
    size?: string;
}

let rootProjectPath = DEFAULT_PROJECT_PATH;
let currentFolderPath = DEFAULT_PROJECT_PATH;
let scannedAssetsList: AssetFile[] = [];
let activeCategoryFilter = "all";
let searchQuery = "";
let renamingAssetPath: string | null = null;
let contextTargetAsset: AssetFile | null = null;
let isCommittingRename = false;

function isPathEqual(p1?: string | null, p2?: string | null): boolean {
    if (!p1 || !p2) return false;
    return p1.toLowerCase().replace(/\//g, '\\') === p2.toLowerCase().replace(/\//g, '\\');
}

export function getCleanSceneDisplayName(rawName: string): string {
    const lower = rawName.toLowerCase();
    if (lower.endsWith(".scene.json")) {
        return rawName.substring(0, rawName.length - 11);
    }
    if (lower.endsWith(".scene")) {
        return rawName.substring(0, rawName.length - 6);
    }
    return rawName;
}

export function updateTopSceneNameUI(scenePath?: string) {
    const el = document.getElementById("top-scene-name");
    if (!el) return;
    const activePath = scenePath || EngineRegistry.EunoiaEngine_ActiveScenePath || "MainLevel.scene.json";
    const fileName = activePath.substring(activePath.lastIndexOf('\\') + 1);
    el.textContent = getCleanSceneDisplayName(fileName);
}

export function loadRealProjectAssets(targetPath: string = currentFolderPath): AssetFile[] {
    EnsureProjectDirectory(targetPath);

    const scannedItems: ScannedProjectItem[] = ReadProjectDirectoryFiles(targetPath);

    if (!scannedItems || scannedItems.length === 0) {
        return [];
    }

    return scannedItems.map(item => {
        let type: AssetFile["type"] = "other";
        let icon = "📄";
        let displayName = item.name;

        // Resolve or register stable numeric resource ID
        const id = item.resourceId || EngineRegistry.GetOrCreateResourceId?.(item.path) || Math.floor(Date.now() + Math.random() * 1000);

        if (item.isDirectory) {
            type = "folder";
            icon = "📁";
        } else {
            const ext = item.extension.toLowerCase();
            const fullNameLower = item.name.toLowerCase();

            if (fullNameLower.endsWith(".scene.json") || ext === ".scene") {
                type = "scene";
                icon = "🎬";
                displayName = getCleanSceneDisplayName(item.name);
            } else if ([".mesh", ".gltf", ".glb", ".obj"].includes(ext)) {
                type = "mesh";
                icon = "📦";
            } else if ([".mat", ".material", ".json"].includes(ext)) {
                type = "material";
                icon = "🎨";
            } else if ([".png", ".jpg", ".jpeg", ".tga", ".dds", ".hdr"].includes(ext)) {
                type = "texture";
                icon = "🖼️";
            } else if ([".ts", ".js"].includes(ext)) {
                type = "script";
                icon = "📜";
            }
        }

        return {
            id,
            name: item.name,
            displayName,
            type,
            icon,
            path: item.path,
            size: item.size
        };
    });
}

function getRelativeDisplayPath(targetPath: string): string {
    const rootName = rootProjectPath.substring(rootProjectPath.lastIndexOf('\\') + 1) || "Eunoia Projects";
    if (isPathEqual(targetPath, rootProjectPath)) {
        return rootName;
    }
    if (targetPath.toLowerCase().startsWith(rootProjectPath.toLowerCase())) {
        const relative = targetPath.substring(rootProjectPath.length);
        const cleanRel = relative.startsWith('\\') ? relative.substring(1) : relative;
        return `${rootName}\\${cleanRel}`;
    }
    return targetPath;
}

function generateUniqueFolderName(targetPath: string): string {
    const existingNames = scannedAssetsList.map(a => a.name.toLowerCase());
    let baseName = "NewFolder";
    if (!existingNames.includes(baseName.toLowerCase())) return baseName;

    let counter = 1;
    while (existingNames.includes(`${baseName}_${counter}`.toLowerCase())) {
        counter++;
    }
    return `${baseName}_${counter}`;
}

function generateUniqueSceneName(targetPath: string): string {
    const existingNames = scannedAssetsList.map(a => a.displayName.toLowerCase());
    let baseName = "NewScene";
    if (!existingNames.includes(baseName.toLowerCase())) return baseName;

    let counter = 1;
    while (existingNames.includes(`newscene_${counter}`.toLowerCase())) {
        counter++;
    }
    return `NewScene_${counter}`;
}

function startRenamingAsset(path: string) {
    renamingAssetPath = path;
    renderAssetsGrid();
}

function commitAssetRename(asset: AssetFile, inputVal: string) {
    if (isCommittingRename) return;
    if (!renamingAssetPath || !isPathEqual(renamingAssetPath, asset.path)) return;
    isCommittingRename = true;

    const cleanInput = inputVal.trim();
    renamingAssetPath = null;

    try {
        if (cleanInput) {
            let targetFileName = cleanInput;
            if (asset.type === "scene") {
                if (!targetFileName.toLowerCase().endsWith(".scene.json")) {
                    targetFileName = `${targetFileName}.scene.json`;
                }
            }

            const currentPath = EngineRegistry.ResolveResourceIdToAbsolutePath?.(asset.id) || asset.path;
            const currentFileName = currentPath.substring(currentPath.lastIndexOf('\\') + 1);

            if (targetFileName.toLowerCase() !== currentFileName.toLowerCase()) {
                const parentDir = currentPath.substring(0, currentPath.lastIndexOf('\\'));
                const newPath = `${parentDir}\\${targetFileName}`;

                const renamed = RenameProjectItem(currentPath, targetFileName);
                if (renamed) {
                    // Update Reference System mapping for stable ID
                    EngineRegistry.UpdateResourcePath?.(currentPath, newPath);

                    if (isPathEqual(currentPath, EngineRegistry.EunoiaEngine_ActiveScenePath)) {
                        EngineRegistry.EunoiaEngine_ActiveScenePath = newPath;
                        if (typeof localStorage !== 'undefined') {
                            localStorage.setItem("EunoiaEngine_LastActiveScene", newPath);
                        }
                        updateTopSceneNameUI(newPath);
                    }
                    showSaveToast(`Renamed to ${getCleanSceneDisplayName(targetFileName)}`);
                }
            }
        }
    } catch (err) {
        console.warn("EUNOIAENGINE > Rename asset error:", err);
    } finally {
        isCommittingRename = false;
        refreshAssetBrowser();
    }
}

export function showSaveToast(msg: string = "Scene Saved") {
    const toast = document.getElementById("save-toast");
    const msgEl = document.getElementById("save-toast-msg");
    if (msgEl) msgEl.textContent = msg;
    if (toast) {
        toast.classList.add("active");
        setTimeout(() => {
            toast.classList.remove("active");
        }, 2000);
    }
}

export function navigateToFolderPath(targetPath: string) {
    currentFolderPath = targetPath;

    const projectPathText = document.getElementById("asset-project-path");
    const backBtn = document.getElementById("btn-browser-back");

    if (projectPathText) {
        projectPathText.textContent = getRelativeDisplayPath(currentFolderPath);
        projectPathText.title = currentFolderPath;
    }

    if (backBtn) {
        const isAtRoot = isPathEqual(currentFolderPath, rootProjectPath);
        backBtn.style.display = isAtRoot ? "none" : "inline-flex";
    }

    refreshAssetBrowser();
}

function setupContextMenu() {
    const browserPanel = document.getElementById("asset-browser-panel");
    const ctxMenu = document.getElementById("asset-context-menu");
    const newFolderBtn = document.getElementById("ctx-new-folder");
    const newSceneBtn = document.getElementById("ctx-new-scene");
    const renameBtn = document.getElementById("ctx-rename");
    const deleteBtn = document.getElementById("ctx-delete");
    const refreshBtn = document.getElementById("ctx-refresh");

    if (!browserPanel || !ctxMenu) return;

    browserPanel.addEventListener("contextmenu", (e) => {
        e.preventDefault();
        e.stopPropagation();

        const target = e.target as HTMLElement;
        const card = target.closest(".asset-card") as HTMLElement;

        if (card && card.dataset.id) {
            const cardId = Number(card.dataset.id);
            contextTargetAsset = scannedAssetsList.find(a => a.id === cardId) || null;
        } else if (card && card.dataset.path) {
            contextTargetAsset = scannedAssetsList.find(a => isPathEqual(a.path, card.dataset.path)) || null;
        } else {
            contextTargetAsset = null;
        }

        if (renameBtn) {
            renameBtn.style.display = contextTargetAsset ? "flex" : "none";
        }
        if (deleteBtn) {
            deleteBtn.style.display = contextTargetAsset ? "flex" : "none";
        }

        ctxMenu.style.left = `${e.clientX}px`;
        ctxMenu.style.top = `${Math.min(e.clientY - 60, window.innerHeight - 160)}px`;
        ctxMenu.classList.add("active");
    });

    document.addEventListener("click", () => {
        ctxMenu.classList.remove("active");
    });

    newFolderBtn?.addEventListener("click", (e) => {
        e.stopPropagation();
        ctxMenu.classList.remove("active");

        const folderName = generateUniqueFolderName(currentFolderPath);
        const created = CreateProjectFolder(folderName, currentFolderPath);

        if (created) {
            const newFolderPath = `${currentFolderPath}\\${folderName}`;
            scannedAssetsList = loadRealProjectAssets(currentFolderPath);
            startRenamingAsset(newFolderPath);
        }
    });

    newSceneBtn?.addEventListener("click", (e) => {
        e.stopPropagation();
        ctxMenu.classList.remove("active");

        const sceneBaseName = generateUniqueSceneName(currentFolderPath);
        const createdPath = CreateNewSceneFile(sceneBaseName, currentFolderPath);

        if (createdPath) {
            scannedAssetsList = loadRealProjectAssets(currentFolderPath);
            startRenamingAsset(createdPath);
        }
    });

    renameBtn?.addEventListener("click", (e) => {
        e.stopPropagation();
        ctxMenu.classList.remove("active");

        if (contextTargetAsset) {
            const resolvedPath = EngineRegistry.ResolveResourceIdToAbsolutePath?.(contextTargetAsset.id) || contextTargetAsset.path;
            startRenamingAsset(resolvedPath);
        }
    });

    deleteBtn?.addEventListener("click", (e) => {
        e.stopPropagation();
        ctxMenu.classList.remove("active");

        if (contextTargetAsset) {
            const targetPath = EngineRegistry.ResolveResourceIdToAbsolutePath?.(contextTargetAsset.id) || contextTargetAsset.path;
            DeleteProjectItem(targetPath);
            showSaveToast(`Deleted ${contextTargetAsset.displayName}`);
            refreshAssetBrowser();
        }
    });

    refreshBtn?.addEventListener("click", (e) => {
        e.stopPropagation();
        ctxMenu.classList.remove("active");
        refreshAssetBrowser();
    });
}

export function initAssetBrowserUI() {
    const browserPanel = document.getElementById("asset-browser-panel");
    const toggleCollapseBtn = document.getElementById("btn-toggle-browser");
    const searchInput = document.getElementById("asset-search-input") as HTMLInputElement;
    const tabButtons = document.querySelectorAll(".browser-tab");
    const backBtn = document.getElementById("btn-browser-back");

    if (!browserPanel) return;

    rootProjectPath = EngineRegistry.EunoiaEngine_ProjectPath || DEFAULT_PROJECT_PATH;
    currentFolderPath = rootProjectPath;

    // Initialize Top Scene Name Badge UI
    updateTopSceneNameUI();

    // Attach Ctrl + S save shortcut listener
    window.addEventListener("keydown", (e) => {
        if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "s") {
            e.preventDefault();
            const saved = SaveActiveScene();
            if (saved) {
                const activePath = EngineRegistry.EunoiaEngine_ActiveScenePath || "MainLevel.scene.json";
                const sceneFile = activePath.substring(activePath.lastIndexOf('\\') + 1);
                const sceneCleanName = getCleanSceneDisplayName(sceneFile);
                updateTopSceneNameUI(activePath);
                showSaveToast(`Saved ${sceneCleanName}`);
            }
        }
    });

    toggleCollapseBtn?.addEventListener("click", () => {
        browserPanel.classList.toggle("collapsed");
        const isCollapsed = browserPanel.classList.contains("collapsed");
        toggleCollapseBtn.textContent = isCollapsed ? "🔼" : "🔽";
    });

    backBtn?.addEventListener("click", () => {
        if (!isPathEqual(currentFolderPath, rootProjectPath)) {
            const lastSlash = currentFolderPath.lastIndexOf('\\');
            if (lastSlash > 0) {
                const parentPath = currentFolderPath.substring(0, lastSlash);
                if (parentPath.length >= rootProjectPath.length) {
                    navigateToFolderPath(parentPath);
                } else {
                    navigateToFolderPath(rootProjectPath);
                }
            }
        }
    });

    searchInput?.addEventListener("input", (e) => {
        searchQuery = (e.target as HTMLInputElement).value.toLowerCase();
        renderAssetsGrid();
    });

    tabButtons.forEach(btn => {
        btn.addEventListener("click", () => {
            tabButtons.forEach(b => b.classList.remove("active"));
            btn.classList.add("active");
            activeCategoryFilter = btn.getAttribute("data-category") || "all";
            renderAssetsGrid();
        });
    });

    setupContextMenu();
    navigateToFolderPath(rootProjectPath);
}

export function refreshAssetBrowser() {
    scannedAssetsList = loadRealProjectAssets(currentFolderPath);
    renderAssetsGrid();
}

export function renderAssetsGrid() {
    const gridContainer = document.getElementById("asset-grid");
    if (!gridContainer) return;

    gridContainer.innerHTML = "";

    const filteredAssets = scannedAssetsList.filter(asset => {
        const matchesCategory = activeCategoryFilter === "all" || asset.type === activeCategoryFilter;
        const matchesSearch = asset.displayName.toLowerCase().includes(searchQuery) || asset.name.toLowerCase().includes(searchQuery);
        return matchesCategory && matchesSearch;
    });

    if (filteredAssets.length === 0) {
        const emptyDiv = document.createElement("div");
        emptyDiv.className = "asset-grid-empty";
        emptyDiv.textContent = "No asset files found in project folder";
        gridContainer.appendChild(emptyDiv);
        return;
    }

    filteredAssets.forEach(asset => {
        const card = document.createElement("div");
        card.className = "asset-card";
        card.dataset.id = String(asset.id);
        card.dataset.path = asset.path;
        card.title = `[ID: ${asset.id}] ${asset.path}`;
        card.draggable = true;

        // Drag source handler using reference ID
        card.addEventListener("dragstart", (e) => {
            if (isPathEqual(renamingAssetPath, asset.path)) {
                e.preventDefault();
                return;
            }
            const currentPath = EngineRegistry.ResolveResourceIdToAbsolutePath?.(asset.id) || asset.path;
            e.dataTransfer?.setData("application/x-eunoia-id", String(asset.id));
            e.dataTransfer?.setData("text/plain", currentPath);
            if (e.dataTransfer) {
                e.dataTransfer.effectAllowed = "move";
            }
        });

        // Drop target handler (only if target asset is a folder)
        if (asset.type === "folder") {
            card.addEventListener("dragover", (e) => {
                e.preventDefault();
                if (e.dataTransfer) {
                    e.dataTransfer.dropEffect = "move";
                }
                card.classList.add("drag-over");
            });

            card.addEventListener("dragleave", () => {
                card.classList.remove("drag-over");
            });

            card.addEventListener("drop", (e) => {
                e.preventDefault();
                card.classList.remove("drag-over");

                // Resolve source path from drag ID or text/plain
                const sourceIdStr = e.dataTransfer?.getData("application/x-eunoia-id");
                let sourcePath: string | null = null;
                if (sourceIdStr) {
                    const sourceId = Number(sourceIdStr);
                    sourcePath = EngineRegistry.ResolveResourceIdToAbsolutePath?.(sourceId) || null;
                }
                if (!sourcePath) {
                    sourcePath = e.dataTransfer?.getData("text/plain") || null;
                }

                const targetFolderPath = EngineRegistry.ResolveResourceIdToAbsolutePath?.(asset.id) || asset.path;

                if (sourcePath && !isPathEqual(sourcePath, targetFolderPath)) {
                    // Prevent moving a folder into itself or a subfolder of itself
                    if (targetFolderPath.toLowerCase().startsWith(sourcePath.toLowerCase())) {
                        return;
                    }
                    const moved = MoveProjectItem(sourcePath, targetFolderPath);
                    if (moved) {
                        const draggedName = sourcePath.substring(sourcePath.lastIndexOf('\\') + 1);
                        const cleanName = getCleanSceneDisplayName(draggedName);
                        showSaveToast(`Moved ${cleanName} to ${asset.displayName}`);
                        refreshAssetBrowser();
                    }
                }
            });
        }

        const iconSpan = document.createElement("span");
        iconSpan.className = "asset-icon";
        iconSpan.textContent = asset.icon;

        card.appendChild(iconSpan);

        if (isPathEqual(renamingAssetPath, asset.path)) {
            // Inline rename text input mode
            const input = document.createElement("input");
            input.type = "text";
            input.className = "asset-name-input";
            input.value = asset.displayName;

            input.addEventListener("click", (e) => e.stopPropagation());
            input.addEventListener("keydown", (e) => {
                if (e.key === "Enter") {
                    commitAssetRename(asset, input.value);
                } else if (e.key === "Escape") {
                    renamingAssetPath = null;
                    renderAssetsGrid();
                }
            });

            input.addEventListener("blur", () => {
                commitAssetRename(asset, input.value);
            });

            card.appendChild(input);

            setTimeout(() => {
                input.focus();
                input.select();
            }, 50);
        } else {
            const nameSpan = document.createElement("span");
            nameSpan.className = "asset-name";
            nameSpan.textContent = asset.displayName;
            card.appendChild(nameSpan);
        }

        if (asset.size) {
            const sizeSpan = document.createElement("span");
            sizeSpan.className = "asset-size";
            sizeSpan.textContent = asset.size;
            card.appendChild(sizeSpan);
        }

        card.addEventListener("click", () => {
            document.querySelectorAll(".asset-card").forEach(c => c.classList.remove("selected"));
            card.classList.add("selected");
        });

        // Double Click to open subfolder or load scene via resolved Reference ID
        card.addEventListener("dblclick", async () => {
            const resolvedPath = EngineRegistry.ResolveResourceIdToAbsolutePath?.(asset.id) || asset.path;
            if (asset.type === "folder") {
                navigateToFolderPath(resolvedPath);
            } else if (asset.type === "scene") {
                const loaded = await LoadSceneFromFile(resolvedPath);
                if (loaded) {
                    updateHierarchyUI();
                    updateTopSceneNameUI(resolvedPath);
                    showSaveToast(`Opened ${asset.displayName}`);
                }
            }
        });

        gridContainer.appendChild(card);
    });
}
