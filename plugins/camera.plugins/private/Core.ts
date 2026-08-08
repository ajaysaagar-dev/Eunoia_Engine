import { FreeCamera, Scene } from "@babylonjs/core";
import ViewportCamera from "./ViewportCamera";
import EngineRegistry from "../../registry.plugins";

EngineRegistry.EunoiaEngine_Camera = null;

async function EunoiaEngine_Camera() {
    ViewportCamera();
    return {
        ActiveCamera: async (camera: FreeCamera) => {
            if (EngineRegistry.EunoiaEngine_Camera) {
                (EngineRegistry.EunoiaEngine_Camera as FreeCamera).dispose();
            }

            EngineRegistry.EunoiaEngine_Camera = camera;
            if (EngineRegistry.EunoiaEngine_Scene) {
                (EngineRegistry.EunoiaEngine_Scene as Scene).activeCamera = camera;
            }
        }
    }
}

export default EunoiaEngine_Camera;