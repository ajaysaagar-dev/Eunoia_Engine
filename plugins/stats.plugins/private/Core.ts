import { Engine, Epsilon } from "@babylonjs/core";
import EngineRegistry from "../../registry.plugins";

const FPS = typeof document !== 'undefined' ? document.getElementById('fps') as HTMLElement : null;

const EunoiaEngine_Stats = {
    ShowFPS: async (show?: boolean) => {
        if (!show) show = !show;
        EngineRegistry.EunoiaEngine_Renderers ??= [];
        if (show) {
            EngineRegistry.EunoiaEngine_Renderers.push(UpdateFPS);
        } else {
            EngineRegistry.EunoiaEngine_Renderers = EngineRegistry.EunoiaEngine_Renderers.filter(item => item.name !== 'UpdateFPS');
            if (FPS) FPS.innerHTML = '';
        }
    }
}

function UpdateFPS() {
    if (FPS && EngineRegistry.EunoiaEngine_Engine) {
        FPS.innerHTML = (EngineRegistry.EunoiaEngine_Engine as Engine).getFps().toFixed(0);
    }
}

export default EunoiaEngine_Stats;