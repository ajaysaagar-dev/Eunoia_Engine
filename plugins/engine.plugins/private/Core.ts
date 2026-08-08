import { Engine, WebGPUEngine } from '@babylonjs/core';
import ViewportResize from './ViewportResize';
import EngineRegistry from '../../registry.plugins';

EngineRegistry.EunoiaEngine_Viewport = document.getElementById('viewport');
EngineRegistry.EunoiaEngine_Engine = null;
EngineRegistry.EunoiaEngine_GraphicsAPI = null;

export default async function EunoiaEngine_Engine(Engine__?: 'WEB_GL' | 'WEB_GPU') {

    if (EngineRegistry.EunoiaEngine_Engine)
        EngineRegistry.EunoiaEngine_Engine.dispose();

    if (!Engine__) {
        EngineRegistry.EunoiaEngine_Engine = new Engine(EngineRegistry.EunoiaEngine_Viewport as HTMLCanvasElement);
        EngineLog(Engine__);
        EngineRegistry.EunoiaEngine_GraphicsAPI = Engine__;
        return;
    }

    if (Engine__ === 'WEB_GL') {
        EngineRegistry.EunoiaEngine_Engine = new Engine(EngineRegistry.EunoiaEngine_Viewport as HTMLCanvasElement);
        EngineLog(Engine__);
        EngineRegistry.EunoiaEngine_GraphicsAPI = Engine__;
        return;
    }

    if (Engine__ === 'WEB_GPU') {
        EngineRegistry.EunoiaEngine_Engine = new WebGPUEngine(EngineRegistry.EunoiaEngine_Viewport as HTMLCanvasElement);
        await (EngineRegistry.EunoiaEngine_Engine as WebGPUEngine).initAsync();
        EngineLog(Engine__);
        EngineRegistry.EunoiaEngine_GraphicsAPI = Engine__;
        return;
    }

}

if (typeof window !== 'undefined') {
    window.addEventListener('resize', () => {
        ViewportResize();
    });
}

function EngineLog(engine: any) {
    console.log(`EUNOIAENGINE > Engine > ${engine} Initialized`);
}