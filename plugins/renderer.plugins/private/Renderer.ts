export const rendererScripts: Renderer[] = [];
let isGameStarted = false;

export function registerRendererScript(script: Renderer) {
    if (!rendererScripts.includes(script)) {
        rendererScripts.push(script);
    }
}

export function unregisterRendererScript(script: Renderer) {
    const index = rendererScripts.indexOf(script);
    if (index !== -1) {
        rendererScripts.splice(index, 1);
    }
}

export function processGameStart() {
    if (isGameStarted) return;
    isGameStarted = true;

    const len = rendererScripts.length;
    for (let i = 0; i < len; i++) {
        const script = rendererScripts[i];
        if (!script.hasStarted) {
            script.hasStarted = true;
            script.GameStart();
        }
    }
}

export function processGameUpdate() {
    const len = rendererScripts.length;
    for (let i = 0; i < len; i++) {
        const script = rendererScripts[i];
        script.GameUpdate();
    }
}

export function resetGameScriptsState() {
    isGameStarted = false;
    const len = rendererScripts.length;
    for (let i = 0; i < len; i++) {
        rendererScripts[i].hasStarted = false;
    }
}

export class Renderer {
    public hasStarted: boolean = false;

    constructor() {
        registerRendererScript(this);
    }

    public GameStart(): void | Promise<void> {
        // To be overridden by child class
    }

    public GameUpdate(): void {
        // To be overridden by child class
    }

    public Destroy(): void {
        unregisterRendererScript(this);
    }
}

export default Renderer;
