import { Scene } from "@babylonjs/core";

(window as any).EunoiaEngine_Scene = null!;

export default async function EunoiaEngine_Scene(scene?: string) {

    if ((window as any).EunoiaEngine_Scene !== null)
        ((window as any).EunoiaEngine_Scene as Scene).dispose();

    if (!scene) {
        (window as any).EunoiaEngine_Scene = new Scene((window as any).EunoiaEngine_Engine);
        SceneLog(scene ? scene : 'ID827EMNCNQCBNJ782987HSDM');
        return;
    }

    return;

}


function SceneLog(scene: any) {
    console.log(`EUNOIAENGINE > Scene > ${scene} Initialized`);
}