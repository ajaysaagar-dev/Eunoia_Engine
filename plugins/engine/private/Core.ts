
import { Engine, WebGPUEngine } from '@babylonjs/core';
import ViewportResize from './ViewportResize';

(window as any).EunoiaEngine_Viewport = document.getElementById('viewport');
(window as any).EunoiaEngine_Engine = null!;
(window as any).EunoiaEngine_GraphicsAPI = null!;

export default async function EunoiaEngine_Engine(Engine__?: 'WEB_GL' | 'WEB_GPU') {

    if ((window as any).EunoiaEngine_Engine !== null)
        (window as any).EunoiaEngine_Engine.dispose();

    if (!Engine__) {
        (window as any).EunoiaEngine_Engine = new Engine((window as any).EunoiaEngine_Viewport as HTMLCanvasElement);
        EngineLog(Engine__);
        (window as any).EunoiaEngine_GraphicsAPI = Engine__;
        return;
    }

    if (Engine__ === 'WEB_GL') {
        (window as any).EunoiaEngine_Engine = new Engine((window as any).EunoiaEngine_Viewport as HTMLCanvasElement);
        EngineLog(Engine__);
        (window as any).EunoiaEngine_GraphicsAPI = Engine__;
        return;
    }

    if (Engine__ === 'WEB_GPU') {
        (window as any).EunoiaEngine_Engine = new WebGPUEngine((window as any).EunoiaEngine_Viewport as HTMLCanvasElement);
        await ((window as any).EunoiaEngine_Engine as WebGPUEngine).initAsync();
        EngineLog(Engine__);
        (window as any).EunoiaEngine_GraphicsAPI = Engine__;
        return;
    }

}

(window as any).addEventListener('resize', () => {
    ViewportResize();
});

function EngineLog(engine: any) {
    console.log(`EUNOIAENGINE > Engine > ${engine} Initialized`);
}