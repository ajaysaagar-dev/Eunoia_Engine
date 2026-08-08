import EngineRegistry from "../../../engine/registry.plugins";
import {
    InitReferences,
    LoadReferences,
    SaveReferences,
    RegisterFile,
    GetOrCreateIdForPath,
    ResolveIdToPath,
    ResolveIdToAbsolutePath,
    ResolvePathToId,
    UpdatePathForId,
    UpdatePath,
    RemoveReference,
    GetMissingReferences,
    ScanAndSyncProjectFiles,
    GetAllReferences,
    type ReferenceEntry
} from "./ReferenceRegistry";

export function InitReferencesPlugin(projectPath?: string): void {
    const rootPath = projectPath || EngineRegistry.EunoiaEngine_ProjectPath || "C:\\Users\\Windows\\Documents\\Eunoia Projects";
    InitReferences(rootPath);
    ScanAndSyncProjectFiles(rootPath);

    // Bind Registry Helpers
    EngineRegistry.GetOrCreateResourceId = (pathStr: string) => GetOrCreateIdForPath(pathStr);
    EngineRegistry.ResolveResourceIdToPath = (id: number) => ResolveIdToPath(id);
    EngineRegistry.ResolveResourceIdToAbsolutePath = (id: number) => ResolveIdToAbsolutePath(id);
    EngineRegistry.ResolveResourcePathToId = (pathStr: string) => ResolvePathToId(pathStr);
    EngineRegistry.RegisterResource = (pathStr: string) => RegisterFile(pathStr);
    EngineRegistry.UpdateResourcePath = (oldPath: string, newPath: string) => UpdatePath(oldPath, newPath);
    EngineRegistry.RemoveResourceReference = (idOrPath: number | string) => RemoveReference(idOrPath);
    EngineRegistry.SaveResourceReferences = () => SaveReferences();

    console.log("EUNOIAENGINE > References > Plugin Initialized with stable numeric ID architecture");
}

const EunoiaEngine_References = {
    Init: InitReferencesPlugin,
    Load: LoadReferences,
    Save: SaveReferences,
    RegisterFile,
    GetOrCreateIdForPath,
    ResolveIdToPath,
    ResolveIdToAbsolutePath,
    ResolvePathToId,
    UpdatePathForId,
    UpdatePath,
    RemoveReference,
    GetMissingReferences,
    ScanAndSyncProjectFiles,
    GetAllReferences
};

export default EunoiaEngine_References;
