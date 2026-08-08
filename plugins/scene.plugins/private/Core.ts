import { Scene } from "@babylonjs/core";
import EngineRegistry from "../../../engine/registry.plugins";

export default async function EunoiaEngine_Scene(sceneName?: string) {

    if (EngineRegistry.EunoiaEngine_Scene) {
        (EngineRegistry.EunoiaEngine_Scene as Scene).dispose();
    }

    if (!sceneName) {
        const scene = new Scene(EngineRegistry.EunoiaEngine_Engine);
        scene.skipPointerMovePicking = true; // Optimization: avoid automatic scene raycasting on pointer movement
        EngineRegistry.EunoiaEngine_Scene = scene;
        SceneLog('ID827EMNCNQCBNJ782987HSDM');
        return;
    }

}

function SceneLog(scene: any) {
    console.log(`EUNOIAENGINE > Scene > ${scene} Initialized`);
}