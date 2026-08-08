
export default function ViewportResize() {
    ((window as any).EunoiaEngine_Viewport as HTMLCanvasElement).width = innerWidth;
    ((window as any).EunoiaEngine_Viewport as HTMLCanvasElement).height = innerHeight;
}