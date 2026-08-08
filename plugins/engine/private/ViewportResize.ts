

export default function ViewportResize() {
    ((window as any).EunoiaEngine_Viewport as HTMLCanvasElement).width = (window as any).innerWidth;
    ((window as any).EunoiaEngine_Viewport as HTMLCanvasElement).height = (window as any).innerHeight;
}