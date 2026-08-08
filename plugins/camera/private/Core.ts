import { FreeCamera, Scene } from "@babylonjs/core";
import ViewportCamera, { ViewportControlsDisable } from "./ViewportCamera";

(window as any).EunoiaEngine_Camera = null!;

export default async function EunoiaEngine_Camera(camera?: FreeCamera) {

    if ((window as any).EunoiaEngine_Camera !== null)
        ((window as any).EunoiaEngine_Camera as FreeCamera).dispose();

    ViewportControlsDisable();

    if (!camera) {
        ViewportCamera();
        return;
    }

    if (camera) {
        (window as any).EunoiaEngine_Camera = camera;
        ((window as any).EunoiaEngine_Scene as Scene).activeCamera = camera;
        return;
    }

}