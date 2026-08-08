import EunoiaEngine_References, { InitReferencesPlugin } from "./private/Core";
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
} from "./private/ReferenceRegistry";

export {
    InitReferencesPlugin,
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
};

export default EunoiaEngine_References;
