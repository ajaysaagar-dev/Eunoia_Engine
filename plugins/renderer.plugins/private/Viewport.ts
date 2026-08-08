import EngineRegistry from "../../registry.plugins";

export default function ViewportResize() {
    if (EngineRegistry.EunoiaEngine_Viewport) {
        (EngineRegistry.EunoiaEngine_Viewport as HTMLCanvasElement).width = window.innerWidth;
        (EngineRegistry.EunoiaEngine_Viewport as HTMLCanvasElement).height = window.innerHeight;
    }
}