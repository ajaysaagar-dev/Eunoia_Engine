import { Engine, Epsilon } from "@babylonjs/core";

const FPS = document.getElementById('fps') as HTMLElement;

const EunoiaEngine_Stats = {
    ShowFPS: async (show?: boolean) => {
        if (!show) show = !show;
        if (show) ((window as any).EunoiaEngine_Renderers as any[]).push(UpdateFPS);
        else {
            ((window as any).EunoiaEngine_Renderers as any[]).filter(item => item.name !== 'UpdateFPS');
            FPS.innerHTML = '';
        }
    }
}

function UpdateFPS() {
    FPS.innerHTML = ((window as any).EunoiaEngine_Engine as Engine).getFps().toFixed(0);
}

export default EunoiaEngine_Stats;