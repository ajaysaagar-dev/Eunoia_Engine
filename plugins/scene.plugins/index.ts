import EunoiaEngine_Scene from "./private/Core";
import { TakeSceneSnapshot, RestoreSceneSnapshot } from "./private/Snapshot";
import {
    SerializeCurrentScene,
    SaveActiveScene,
    CreateNewSceneFile,
    ClearSceneUserObjects,
    LoadSceneFromFile,
    GetLastOpenedScenePath
} from "./private/Serialization";

export {
    TakeSceneSnapshot,
    RestoreSceneSnapshot,
    SerializeCurrentScene,
    SaveActiveScene,
    CreateNewSceneFile,
    ClearSceneUserObjects,
    LoadSceneFromFile,
    GetLastOpenedScenePath
};

export default EunoiaEngine_Scene;