import { Scene } from "@babylonjs/core";
import EngineRegistry from "../../registry.plugins";

EngineRegistry.EunoiaEngine_Scene = null;

export default async function EunoiaEngine_Scene(scene?: string) {

    if (EngineRegistry.EunoiaEngine_Scene)
        (EngineRegistry.EunoiaEngine_Scene as Scene).dispose();

    if (!scene) {
        EngineRegistry.EunoiaEngine_Scene = new Scene(EngineRegistry.EunoiaEngine_Engine);
        SceneLog(scene ? scene : 'ID827EMNCNQCBNJ782987HSDM');
        return;
    }

    return;

}

function SceneLog(scene: any) {
    console.log(`EUNOIAENGINE > Scene > ${scene} Initialized`);
}