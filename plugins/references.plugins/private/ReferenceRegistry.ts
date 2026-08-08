import EngineRegistry from "../../../engine/registry.plugins";

const fs = require("fs-extra");

export interface ReferenceEntry {
    id: number;
    path: string; // Relative path from project root (e.g. "assets/models/player.glb")
}

let activeProjectPath: string = "C:\\Users\\Windows\\Documents\\Eunoia Projects";
let referencesMap: Map<number, string> = new Map(); // id -> relative path
let pathToIdMap: Map<string, number> = new Map(); // relative path -> id

function normalizeRelativePath(relPath: string): string {
    return relPath.replace(/\\/g, "/").replace(/^\/+/, "").trim();
}

function getRelativePath(absoluteOrRelativePath: string, rootDir: string): string {
    let cleanPath = absoluteOrRelativePath.replace(/\\/g, "/");
    let cleanRoot = rootDir.replace(/\\/g, "/");
    if (!cleanRoot.endsWith("/")) cleanRoot += "/";

    if (cleanPath.toLowerCase().startsWith(cleanRoot.toLowerCase())) {
        cleanPath = cleanPath.substring(cleanRoot.length);
    }
    return normalizeRelativePath(cleanPath);
}

function getAbsolutePath(relativePath: string, rootDir: string): string {
    const normRel = relativePath.replace(/\//g, "\\");
    const normRoot = rootDir.endsWith("\\") ? rootDir : `${rootDir}\\`;
    return `${normRoot}${normRel}`;
}

function generateUniqueNumericId(): number {
    let id: number;
    do {
        // Generate a stable 12-digit unique numeric ID (e.g., 123981764892)
        id = Math.floor(100000000000 + Math.random() * 900000000000);
    } while (referencesMap.has(id));
    return id;
}

export function GetReferencesFilePath(projectPath: string = activeProjectPath): string {
    const root = projectPath || EngineRegistry.EunoiaEngine_ProjectPath || activeProjectPath;
    const cleanRoot = root.endsWith("\\") ? root.substring(0, root.length - 1) : root;
    return `${cleanRoot}\\references.json`;
}

export function InitReferences(projectPath?: string): void {
    if (projectPath) {
        activeProjectPath = projectPath;
    } else if (EngineRegistry.EunoiaEngine_ProjectPath) {
        activeProjectPath = EngineRegistry.EunoiaEngine_ProjectPath;
    }

    try {
        if (typeof window !== "undefined" && fs && fs.ensureDirSync) {
            fs.ensureDirSync(activeProjectPath);
        }
    } catch (e) { }

    LoadReferences();
}

export function LoadReferences(projectPath?: string): ReferenceEntry[] {
    if (projectPath) activeProjectPath = projectPath;
    const refFilePath = GetReferencesFilePath(activeProjectPath);

    referencesMap.clear();
    pathToIdMap.clear();

    try {
        if (typeof window !== "undefined" && fs && fs.existsSync) {
            if (fs.existsSync(refFilePath)) {
                const content = fs.readFileSync(refFilePath, "utf-8");
                const data: ReferenceEntry[] = JSON.parse(content);

                if (Array.isArray(data)) {
                    data.forEach((entry) => {
                        if (entry && typeof entry.id === "number" && typeof entry.path === "string") {
                            const normPath = normalizeRelativePath(entry.path);
                            referencesMap.set(entry.id, normPath);
                            pathToIdMap.set(normPath.toLowerCase(), entry.id);
                        }
                    });
                }
            } else {
                // Initialize empty references.json file as []
                fs.writeFileSync(refFilePath, JSON.stringify([], null, 2), "utf-8");
            }
        }
    } catch (err) {
        console.warn("EUNOIAENGINE > References > Failed to load references.json:", err);
    }

    return GetAllReferences();
}

export function SaveReferences(projectPath?: string): void {
    if (projectPath) activeProjectPath = projectPath;
    const refFilePath = GetReferencesFilePath(activeProjectPath);

    const entries: ReferenceEntry[] = GetAllReferences();

    try {
        if (typeof window !== "undefined" && fs && fs.writeFileSync) {
            fs.writeFileSync(refFilePath, JSON.stringify(entries, null, 2), "utf-8");
        }
    } catch (err) {
        console.error("EUNOIAENGINE > References > Failed to save references.json:", err);
    }
}

export function GetAllReferences(): ReferenceEntry[] {
    const list: ReferenceEntry[] = [];
    referencesMap.forEach((relPath, id) => {
        list.push({ id, path: relPath });
    });
    return list;
}

export function RegisterFile(absoluteOrRelativePath: string): ReferenceEntry {
    const relPath = getRelativePath(absoluteOrRelativePath, activeProjectPath);
    if (!relPath) {
        return { id: 0, path: "" };
    }

    const lowerRel = relPath.toLowerCase();
    const existingId = pathToIdMap.get(lowerRel);

    if (existingId !== undefined) {
        return { id: existingId, path: relPath };
    }

    const newId = generateUniqueNumericId();
    referencesMap.set(newId, relPath);
    pathToIdMap.set(lowerRel, newId);

    SaveReferences();
    return { id: newId, path: relPath };
}

export function GetOrCreateIdForPath(absoluteOrRelativePath: string): number {
    const entry = RegisterFile(absoluteOrRelativePath);
    return entry.id;
}

export function ResolveIdToPath(id: number): string | null {
    return referencesMap.get(id) || null;
}

export function ResolveIdToAbsolutePath(id: number): string | null {
    const rel = referencesMap.get(id);
    if (!rel) return null;
    return getAbsolutePath(rel, activeProjectPath);
}

export function ResolvePathToId(absoluteOrRelativePath: string): number | null {
    const relPath = getRelativePath(absoluteOrRelativePath, activeProjectPath);
    return pathToIdMap.get(relPath.toLowerCase()) || null;
}

export function UpdatePathForId(id: number, newAbsoluteOrRelativePath: string): boolean {
    if (!referencesMap.has(id)) return false;

    const oldRelPath = referencesMap.get(id)!;
    pathToIdMap.delete(oldRelPath.toLowerCase());

    const newRelPath = getRelativePath(newAbsoluteOrRelativePath, activeProjectPath);
    referencesMap.set(id, newRelPath);
    pathToIdMap.set(newRelPath.toLowerCase(), id);

    SaveReferences();
    return true;
}

export function UpdatePath(oldAbsoluteOrRelativePath: string, newAbsoluteOrRelativePath: string): boolean {
    const oldRelPath = getRelativePath(oldAbsoluteOrRelativePath, activeProjectPath);
    const newRelPath = getRelativePath(newAbsoluteOrRelativePath, activeProjectPath);
    const oldLower = oldRelPath.toLowerCase();
    const oldPrefix = oldLower.endsWith("/") ? oldLower : `${oldLower}/`;

    let updatedAny = false;

    // Direct match update
    const directId = pathToIdMap.get(oldLower);
    if (directId !== undefined) {
        UpdatePathForId(directId, newAbsoluteOrRelativePath);
        updatedAny = true;
    }

    // Prefix matches for directory rename
    const entriesToUpdate: Array<{ id: number; newPath: string }> = [];
    pathToIdMap.forEach((id, existingPathLower) => {
        if (existingPathLower.startsWith(oldPrefix)) {
            const suffix = existingPathLower.substring(oldPrefix.length);
            const updatedRel = `${newRelPath}/${suffix}`;
            entriesToUpdate.push({ id, newPath: updatedRel });
        }
    });

    entriesToUpdate.forEach(({ id, newPath }) => {
        UpdatePathForId(id, newPath);
        updatedAny = true;
    });

    if (!updatedAny) {
        RegisterFile(newAbsoluteOrRelativePath);
    }

    SaveReferences();
    return true;
}

export function RemoveReference(idOrPath: number | string): boolean {
    let id: number | null = null;
    if (typeof idOrPath === "number") {
        id = idOrPath;
    } else {
        id = ResolvePathToId(idOrPath);
    }

    if (id !== null && referencesMap.has(id)) {
        const relPath = referencesMap.get(id)!;
        referencesMap.delete(id);
        pathToIdMap.delete(relPath.toLowerCase());
        SaveReferences();
        return true;
    }
    return false;
}

export function GetMissingReferences(): ReferenceEntry[] {
    const missing: ReferenceEntry[] = [];
    referencesMap.forEach((relPath, id) => {
        const absPath = getAbsolutePath(relPath, activeProjectPath);
        try {
            if (typeof window !== "undefined" && fs && fs.existsSync) {
                if (!fs.existsSync(absPath)) {
                    missing.push({ id, path: relPath });
                }
            }
        } catch (e) { }
    });
    return missing;
}

export function ScanAndSyncProjectFiles(projectPath?: string): ReferenceEntry[] {
    if (projectPath) activeProjectPath = projectPath;

    try {
        if (typeof window !== "undefined" && fs && fs.readdirSync) {
            const scanDirectory = (dir: string) => {
                const items: string[] = fs.readdirSync(dir);
                items.forEach((item: string) => {
                    if (item === "references.json" || item.startsWith(".")) return;
                    const fullPath = `${dir}\\${item}`;
                    try {
                        const stat = fs.statSync(fullPath);
                        if (stat.isDirectory()) {
                            scanDirectory(fullPath);
                        } else {
                            RegisterFile(fullPath);
                        }
                    } catch (e) { }
                });
            };

            if (fs.existsSync(activeProjectPath)) {
                scanDirectory(activeProjectPath);
            }
        }
    } catch (err) {
        console.warn("EUNOIAENGINE > References > Sync scan error:", err);
    }

    return GetAllReferences();
}
