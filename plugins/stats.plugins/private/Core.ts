import { Engine } from "@babylonjs/core";
import EngineRegistry from "../../../engine/registry.plugins";

let FPS: HTMLElement | null = null;
let lastFpsText = '';
let lastUpdateTime = 0;

function getFPSElement(): HTMLElement | null {
    if (!FPS && typeof document !== 'undefined') {
        FPS = document.getElementById('fps');
    }
    return FPS;
}

const EunoiaEngine_Stats = {
    ShowFPS: async (show?: boolean) => {
        if (!show) show = !show;
        EngineRegistry.EunoiaEngine_Renderers ??= [];
        if (show) {
            if (!EngineRegistry.EunoiaEngine_Renderers.includes(UpdateFPS)) {
                EngineRegistry.EunoiaEngine_Renderers.push(UpdateFPS);
            }
        } else {
            EngineRegistry.EunoiaEngine_Renderers = EngineRegistry.EunoiaEngine_Renderers.filter(item => item !== UpdateFPS);
            const el = getFPSElement();
            if (el) el.textContent = '';
        }
    }
}

function UpdateFPS() {
    const now = performance.now();
    // Throttle DOM updates to 4 times per second (every 250ms)
    if (now - lastUpdateTime < 250) return;
    lastUpdateTime = now;

    const engine = EngineRegistry.EunoiaEngine_Engine as Engine;
    const el = getFPSElement();
    if (el && engine) {
        const currentFps = engine.getFps().toFixed(0);
        if (currentFps !== lastFpsText) {
            lastFpsText = currentFps;
            el.textContent = currentFps;
        }
    }
}

export default EunoiaEngine_Stats;