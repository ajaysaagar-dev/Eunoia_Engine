import { Engine, Scene } from "@babylonjs/core";
import ViewportResize from "../../engine.plugins/private/ViewportResize";
import EngineRegistry from "../../registry.plugins";

EngineRegistry.EunoiaEngine_Renderer = null;
EngineRegistry.EunoiaEngine_Renderers ??= [];

export default async function EunoiaEngine_Renderer() {

    if (EngineRegistry.EunoiaEngine_Renderer)
        (EngineRegistry.EunoiaEngine_Renderer as any).dispose();

    (EngineRegistry.EunoiaEngine_Engine as Engine)?.runRenderLoop(() => {
        (EngineRegistry.EunoiaEngine_Renderers as any[])?.forEach(renderer => { renderer() });
        (EngineRegistry.EunoiaEngine_Scene as Scene)?.render();
    });

    ViewportResize();

    RendererLog('Renderer');

}

function RendererLog(renderer: any) {
    console.log(`EUNOIAENGINE > Renderer > ${renderer} Initialized`);
}