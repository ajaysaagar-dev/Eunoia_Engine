const EunoiaEngine_FileSystem = require('fs-extra');
import type * as FS_TYPE from 'fs-extra';
import EngineRegistry from '../../../engine/registry.plugins';

export const DEFAULT_PROJECT_PATH = "C:\\Users\\Windows\\Documents\\Eunoia Projects";

EngineRegistry.EunoiaEngine_DefaultProjectPath = DEFAULT_PROJECT_PATH;
EngineRegistry.EunoiaEngine_ProjectPath ??= DEFAULT_PROJECT_PATH;

export function GetDefaultProjectPath(): string {
    return DEFAULT_PROJECT_PATH;
}

export function GetActiveProjectPath(): string {
    return EngineRegistry.EunoiaEngine_ProjectPath || DEFAULT_PROJECT_PATH;
}

export function SetActiveProjectPath(pathStr: string): void {
    EngineRegistry.EunoiaEngine_ProjectPath = pathStr;
}

export function EnsureProjectDirectory(projectPath: string = DEFAULT_PROJECT_PATH): void {
    try {
        if (typeof window !== 'undefined' && EunoiaEngine_FileSystem && EunoiaEngine_FileSystem.ensureDirSync) {
            EunoiaEngine_FileSystem.ensureDirSync(projectPath);
        }
    } catch (err) {
        console.warn("EUNOIAENGINE > FileSystem > Could not auto-create project dir:", err);
    }
}

export function CreateProjectFolder(folderName: string, parentPath: string = GetActiveProjectPath()): boolean {
    try {
        EnsureProjectDirectory(parentPath);
        const folderPath = `${parentPath}\\${folderName}`;
        if (typeof window !== 'undefined' && EunoiaEngine_FileSystem && EunoiaEngine_FileSystem.ensureDirSync) {
            EunoiaEngine_FileSystem.ensureDirSync(folderPath);
            EngineRegistry.RegisterResource?.(folderPath);
            return true;
        }
    } catch (err) {
        console.warn("EUNOIAENGINE > FileSystem > Could not create folder:", err);
    }
    return false;
}

export function RenameProjectItem(oldPath: string, newName: string): boolean {
    try {
        if (typeof window !== 'undefined' && EunoiaEngine_FileSystem && EunoiaEngine_FileSystem.renameSync) {
            const parentDir = oldPath.substring(0, oldPath.lastIndexOf('\\'));
            const newPath = `${parentDir}\\${newName}`;
            if (oldPath !== newPath) {
                EunoiaEngine_FileSystem.renameSync(oldPath, newPath);
                // Maintain stable reference ID and update target path
                EngineRegistry.UpdateResourcePath?.(oldPath, newPath);
                return true;
            }
        }
    } catch (err) {
        console.warn("EUNOIAENGINE > FileSystem > Could not rename item:", err);
    }
    return false;
}

export function MoveProjectItem(sourcePath: string, targetFolderPath: string): boolean {
    try {
        if (typeof window !== 'undefined' && EunoiaEngine_FileSystem && EunoiaEngine_FileSystem.moveSync) {
            const fileName = sourcePath.substring(sourcePath.lastIndexOf('\\') + 1);
            const destinationPath = `${targetFolderPath}\\${fileName}`;
            if (sourcePath.toLowerCase() !== destinationPath.toLowerCase()) {
                EunoiaEngine_FileSystem.moveSync(sourcePath, destinationPath, { overwrite: true });
                // Maintain stable reference ID and update target path
                EngineRegistry.UpdateResourcePath?.(sourcePath, destinationPath);
                return true;
            }
        }
    } catch (err) {
        console.warn("EUNOIAENGINE > FileSystem > Could not move item:", err);
    }
    return false;
}

export function DeleteProjectItem(targetPath: string): boolean {
    try {
        if (typeof window !== 'undefined' && EunoiaEngine_FileSystem && EunoiaEngine_FileSystem.removeSync) {
            EunoiaEngine_FileSystem.removeSync(targetPath);
            // Remove from reference system
            EngineRegistry.RemoveResourceReference?.(targetPath);
            return true;
        }
    } catch (err) {
        console.warn("EUNOIAENGINE > FileSystem > Could not delete item:", err);
    }
    return false;
}

export interface ScannedProjectItem {
    name: string;
    path: string;
    isDirectory: boolean;
    size?: string;
    extension?: string;
    resourceId?: number;
}

export function ReadProjectDirectoryFiles(projectPath: string = GetActiveProjectPath()): ScannedProjectItem[] {
    EnsureProjectDirectory(projectPath);
    const results: ScannedProjectItem[] = [];

    try {
        if (typeof window !== 'undefined' && EunoiaEngine_FileSystem && EunoiaEngine_FileSystem.readdirSync) {
            const items: string[] = EunoiaEngine_FileSystem.readdirSync(projectPath);
            items.forEach((item: string) => {
                if (item === "references.json" || item.startsWith(".")) return;
                const fullPath = `${projectPath}\\${item}`;
                try {
                    const stat = EunoiaEngine_FileSystem.statSync(fullPath);
                    const isDirectory = stat.isDirectory();
                    let sizeStr = "";
                    if (!isDirectory) {
                        const bytes = stat.size;
                        if (bytes < 1024) sizeStr = `${bytes} B`;
                        else if (bytes < 1024 * 1024) sizeStr = `${(bytes / 1024).toFixed(1)} KB`;
                        else sizeStr = `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
                    }
                    const extIndex = item.lastIndexOf('.');
                    const extension = (!isDirectory && extIndex !== -1) ? item.substring(extIndex).toLowerCase() : "";

                    // Register or resolve stable numeric resource ID
                    const resourceId = EngineRegistry.GetOrCreateResourceId?.(fullPath);

                    results.push({
                        name: item,
                        path: fullPath,
                        isDirectory,
                        size: sizeStr,
                        extension,
                        resourceId
                    });
                } catch (e) {
                    // skip unreadable
                }
            });
        }
    } catch (err) {
        console.warn("EUNOIAENGINE > FileSystem > Error reading directory:", err);
    }

    return results;
}

export default EunoiaEngine_FileSystem as typeof FS_TYPE;