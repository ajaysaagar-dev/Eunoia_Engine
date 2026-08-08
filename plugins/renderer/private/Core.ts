import { Engine, Scene } from "@babylonjs/core";
import ViewportResize from "../../engine/private/ViewportResize";

(window as any).EunoiaEngine_Renderer = null!;
(window as any).EunoiaEngine_Renderers = [];

export default async function EunoiaEngine_Renderer() {

    if ((window as any).EunoiaEngine_Renderer !== null)
        ((window as any).EunoiaEngine_Renderer as any).dispose();

    ((window as any).EunoiaEngine_Engine as Engine).runRenderLoop(() => {
        ((window as any).EunoiaEngine_Renderers as any[]).forEach(renderer => { renderer() });
        ((window as any).EunoiaEngine_Scene as Scene).render();
    });

    ViewportResize();

    RendererLog('Renderer');

}

function RendererLog(renderer: any) {
    console.log(`EUNOIAENGINE > Renderer > ${renderer} Initialized`);
}