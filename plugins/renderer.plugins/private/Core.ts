import { Engine, Scene } from "@babylonjs/core";
import ViewportResize from "../../engine.plugins/private/ViewportResize";
import EngineRegistry from "../../../engine/registry.plugins";
import EngineStates from "../../../engine/states.engine";
import { processGameStart, processGameUpdate, resetGameScriptsState } from "./Renderer";

EngineRegistry.EunoiaEngine_Renderer = null;
EngineRegistry.EunoiaEngine_Renderers ??= [];

let isCursorDown = false;
let renderRequestedFrames = 15;
let listenersInitialized = false;
let isLoopRunning = false;
let previousMode: string = 'Editor';

export function RequestRender(frames: number = 15) {
    renderRequestedFrames = Math.max(renderRequestedFrames, frames);
    ensureRenderLoop();
}

EngineRegistry.RequestRender = RequestRender;

function ensureRenderLoop() {
    const engine = EngineRegistry.EunoiaEngine_Engine as Engine;
    if (!engine || isLoopRunning) return;

    isLoopRunning = true;
    engine.runRenderLoop(onRenderLoopTick);
}

function stopRenderLoop() {
    const engine = EngineRegistry.EunoiaEngine_Engine as Engine;
    if (!engine || !isLoopRunning) return;

    isLoopRunning = false;
    engine.stopRenderLoop(onRenderLoopTick);
}

function onRenderLoopTick() {
    const mode = EngineStates.Mode ?? 'Editor';

    // Handle mode state transitions for scripts
    if (previousMode === 'Game' && mode === 'Editor') {
        resetGameScriptsState();
    }
    previousMode = mode;

    if (mode === 'Game') {
        processGameStart();
        processGameUpdate();
        renderFrame();
    } else {
        // Mode === 'Editor': render active interaction or requested countdown frames
        if (isCursorDown || renderRequestedFrames > 0) {
            if (renderRequestedFrames > 0) {
                renderRequestedFrames--;
            }
            renderFrame();
        } else {
            // Stop GPU render loop when Editor is completely idle
            stopRenderLoop();
        }
    }
}

function setupCursorListeners() {
    if (listenersInitialized || typeof window === 'undefined') return;
    listenersInitialized = true;

    const onPointerDown = () => {
        isCursorDown = true;
        RequestRender(15);
    };

    const onPointerUp = () => {
        isCursorDown = false;
        RequestRender(15);
    };

    const onPointerMove = () => {
        if (isCursorDown) {
            RequestRender(15);
        }
    };

    window.addEventListener('mousedown', onPointerDown, { passive: true });
    window.addEventListener('mouseup', onPointerUp, { passive: true });
    window.addEventListener('pointerdown', onPointerDown, { passive: true });
    window.addEventListener('pointerup', onPointerUp, { passive: true });
    window.addEventListener('mousemove', onPointerMove, { passive: true });
    window.addEventListener('pointermove', onPointerMove, { passive: true });
    window.addEventListener('blur', () => {
        isCursorDown = false;
    });
}

export default async function EunoiaEngine_Renderer() {

    if (EngineRegistry.EunoiaEngine_Renderer) {
        (EngineRegistry.EunoiaEngine_Renderer as any).dispose();
    }

    setupCursorListeners();
    RequestRender(15);

    ViewportResize();

    RendererLog('Renderer');

}

function renderFrame() {
    const renderers = EngineRegistry.EunoiaEngine_Renderers as Function[];
    if (renderers) {
        const len = renderers.length;
        for (let i = 0; i < len; i++) {
            renderers[i]();
        }
    }

    const scene = EngineRegistry.EunoiaEngine_Scene as Scene;
    if (scene) {
        scene.render();
    }
}

function RendererLog(renderer: any) {
    console.log(`EUNOIAENGINE > Renderer > ${renderer} Initialized`);
}