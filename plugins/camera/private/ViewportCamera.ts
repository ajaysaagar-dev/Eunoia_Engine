import { Clamp, Engine, EventState, FreeCamera, Observer, PointerEventTypes, PointerInfo, Scene, Space, TransformNode, Vector3 } from "@babylonjs/core";


export default function ViewportCamera() {

    (window as any).EunoiaEngine_Camera_TN = new TransformNode('Viewport_Camera_Parent', (window as any).EunoiaEngine_Scene as Scene);
    (window as any).EunoiaEngine_Camera = new FreeCamera(
        'viewport_camera',
        new Vector3(0, 3, -8),
        (window as any).EunoiaEngine_Scene
    );

    ((window as any).EunoiaEngine_Camera as FreeCamera).parent = ((window as any).EunoiaEngine_Camera_TN);
    ((window as any).EunoiaEngine_Camera_TN as TransformNode).position.set(0, 2, -10);

    ViewportControlsEnable();

}

// Controls

let events: {
    startEvent: any
    endEvent: any
} = {
    startEvent: null!,
    endEvent: null!
}

export function ViewportControlsEnable() {
    document.addEventListener('keydown', KeyDown);
    document.addEventListener('keyup', KeyUp);
    document.addEventListener('mousemove', Look);
    events.startEvent = ((window as any).EunoiaEngine_Scene as Scene).onPointerObservable.add(ViewportLooksControlsDown);
    events.endEvent = ((window as any).EunoiaEngine_Scene as Scene).onPointerObservable.add(ViewportLooksControlsUp);
    ((window as any).EunoiaEngine_Renderers as any[]).push(ViewportControlsUpdate);
    ((window as any).EunoiaEngine_Renderers as any[]).push(ViewportLooksControlsUpdate);
    ((window as any).EunoiaEngine_Camera as FreeCamera).position.set(0, 0, 0);
}

export function ViewportControlsDisable() {
    document.removeEventListener('keydown', KeyDown);
    document.removeEventListener('keyup', KeyUp);
    document.removeEventListener('mousemove', Look);
    ((window as any).EunoiaEngine_Scene as Scene).onPointerObservable.remove(events.startEvent);
    ((window as any).EunoiaEngine_Scene as Scene).onPointerObservable.remove(events.endEvent);
    for (const index in moves) moves[index as keyof typeof moves] = 0;
    (window as any).EunoiaEngine_Renderers =
        ((window as any).EunoiaEngine_Renderers as any[]).filter(renderer => renderer.name !== 'ViewportControlsUpdate');
    (window as any).EunoiaEngine_Renderers =
        ((window as any).EunoiaEngine_Renderers as any[]).filter(renderer => renderer.name !== 'ViewportLooksControlsUpdate');
}

// Look

const looks = {
    x: 0,
    y: 0
};

let looking = false;

function Look(e: MouseEvent) {
    looks.x = e.movementX;
    looks.y = e.movementY;
}

function ViewportLooksControlsDown(pi: PointerInfo, e: EventState) {
    if (pi.event.button === 2 && pi.type === PointerEventTypes.POINTERDOWN) {
        looking = true;
        ((window as any).EunoiaEngine_Viewport as HTMLCanvasElement).requestPointerLock();
    }
}

function ViewportLooksControlsUp(pi: PointerInfo, e: EventState) {
    if (pi.event.button === 2 && pi.type === PointerEventTypes.POINTERUP) {
        looking = false;
        document.exitPointerLock();
    }
}

function ViewportLooksControlsUpdate() {
    if (!looking) return;
    ((window as any).EunoiaEngine_Camera_TN as FreeCamera).rotation.y += looks.x * 0.001 * ((window as any).EunoiaEngine_Engine as Engine).getDeltaTime();
    ((window as any).EunoiaEngine_Camera_TN as FreeCamera).rotation.x += looks.y * 0.001 * ((window as any).EunoiaEngine_Engine as Engine).getDeltaTime();
    ((window as any).EunoiaEngine_Camera as FreeCamera).rotation.z = 0;
    looks.x = looks.y = 0;

}

// Keyboard

const moves = {
    w: 0,
    a: 0,
    s: 0,
    d: 0,
    q: 0,
    e: 0
};

function ViewportControlsUpdate() {
    if (moves.w)
        ((window as any).EunoiaEngine_Camera_TN as TransformNode).translate(Vector3.Forward(), 0.1, Space.LOCAL);
    if (moves.a)
        ((window as any).EunoiaEngine_Camera_TN as TransformNode).translate(Vector3.Left(), 0.1, Space.LOCAL);
    if (moves.s)
        ((window as any).EunoiaEngine_Camera_TN as TransformNode).translate(Vector3.Backward(), 0.1, Space.LOCAL);
    if (moves.d)
        ((window as any).EunoiaEngine_Camera_TN as TransformNode).translate(Vector3.Right(), 0.1, Space.LOCAL);
    if (moves.q)
        ((window as any).EunoiaEngine_Camera_TN as TransformNode).translate(Vector3.Down(), 0.1, Space.WORLD);
    if (moves.e)
        ((window as any).EunoiaEngine_Camera_TN as TransformNode).translate(Vector3.Up(), 0.1, Space.WORLD);
}

function KeyDown(e: KeyboardEvent) {
    if (e.key === 'w') moves.w = 1;
    if (e.key === 'a') moves.a = 1;
    if (e.key === 's') moves.s = 1;
    if (e.key === 'd') moves.d = 1;
    if (e.key === 'q') moves.q = 1;
    if (e.key === 'e') moves.e = 1;
}
function KeyUp(e: KeyboardEvent) {
    if (e.key === 'w') moves.w = 0;
    if (e.key === 'a') moves.a = 0;
    if (e.key === 's') moves.s = 0;
    if (e.key === 'd') moves.d = 0;
    if (e.key === 'q') moves.q = 0;
    if (e.key === 'e') moves.e = 0;
}