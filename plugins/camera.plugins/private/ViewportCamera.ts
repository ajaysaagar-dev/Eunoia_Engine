import { Clamp, Engine, EventState, FreeCamera, Observer, PointerEventTypes, PointerInfo, Scalar, Scene, Space, TransformNode, Vector3 } from "@babylonjs/core";
import EngineRegistry from "../../../engine/registry.plugins";

// Scratch static vectors to prevent GC allocations in hot loop
const _vecForward = Vector3.Forward();
const _vecRight = Vector3.Right();
const _vecUp = Vector3.Up();

export default function ViewportCamera() {

    const scene = EngineRegistry.EunoiaEngine_Scene as Scene;
    EngineRegistry.EunoiaEngine_Camera_TN = new TransformNode('Viewport_Camera_Parent', scene);
    EngineRegistry.EunoiaEngine_Camera = new FreeCamera(
        'viewport_camera',
        new Vector3(0, 3, -8),
        scene
    );

    (EngineRegistry.EunoiaEngine_Camera as FreeCamera).parent = EngineRegistry.EunoiaEngine_Camera_TN as TransformNode;
    const camTN = EngineRegistry.EunoiaEngine_Camera_TN as TransformNode;
    camTN.position.set(0, 2, -10);

    targetRotation.x = camTN.rotation.x;
    targetRotation.y = camTN.rotation.y;
    currentRotation.x = camTN.rotation.x;
    currentRotation.y = camTN.rotation.y;

    ViewportControlsEnable();

    // Trigger initial render when viewport camera loads
    EngineRegistry.RequestRender?.(15);

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
    events.startEvent = (EngineRegistry.EunoiaEngine_Scene as Scene)?.onPointerObservable?.add(ViewportLooksControlsDown);
    events.endEvent = (EngineRegistry.EunoiaEngine_Scene as Scene)?.onPointerObservable?.add(ViewportLooksControlsUp);
    EngineRegistry.EunoiaEngine_Renderers ??= [];
    if (!EngineRegistry.EunoiaEngine_Renderers.includes(ViewportControlsUpdate)) {
        EngineRegistry.EunoiaEngine_Renderers.push(ViewportControlsUpdate);
    }
    if (!EngineRegistry.EunoiaEngine_Renderers.includes(ViewportLooksControlsUpdate)) {
        EngineRegistry.EunoiaEngine_Renderers.push(ViewportLooksControlsUpdate);
    }
    (EngineRegistry.EunoiaEngine_Camera as FreeCamera)?.position?.set(0, 0, 0);
}

export function ViewportControlsDisable() {
    document.removeEventListener('keydown', KeyDown);
    document.removeEventListener('keyup', KeyUp);
    document.removeEventListener('mousemove', Look);
    if (EngineRegistry.EunoiaEngine_Scene) {
        (EngineRegistry.EunoiaEngine_Scene as Scene).onPointerObservable.remove(events.startEvent);
        (EngineRegistry.EunoiaEngine_Scene as Scene).onPointerObservable.remove(events.endEvent);
    }
    for (const index in moves) moves[index as keyof typeof moves] = 0;
    if (EngineRegistry.EunoiaEngine_Renderers) {
        EngineRegistry.EunoiaEngine_Renderers =
            EngineRegistry.EunoiaEngine_Renderers.filter((renderer: any) => renderer !== ViewportControlsUpdate && renderer !== ViewportLooksControlsUpdate);
    }
}

// Look & Rotation Damping

const targetRotation = { x: 0, y: 0 };
const currentRotation = { x: 0, y: 0 };
const lookSensitivity = 0.002;
const rotationDampingSpeed = 18;

let looking = false;

function Look(e: MouseEvent) {
    if (!looking) return;
    targetRotation.y += e.movementX * lookSensitivity;
    targetRotation.x += e.movementY * lookSensitivity;

    const maxPitch = Math.PI / 2 - 0.02;
    targetRotation.x = Math.max(-maxPitch, Math.min(maxPitch, targetRotation.x));
    EngineRegistry.RequestRender?.(10);
}

function ViewportLooksControlsDown(pi: PointerInfo, e: EventState) {
    if (pi.event.button === 2 && pi.type === PointerEventTypes.POINTERDOWN) {
        looking = true;
        (EngineRegistry.EunoiaEngine_Viewport as HTMLCanvasElement)?.requestPointerLock();
        EngineRegistry.RequestRender?.(15);
    }
}

function ViewportLooksControlsUp(pi: PointerInfo, e: EventState) {
    if (pi.event.button === 2 && pi.type === PointerEventTypes.POINTERUP) {
        looking = false;
        document.exitPointerLock();
        EngineRegistry.RequestRender?.(15);
    }
}

function ViewportLooksControlsUpdate() {
    const camTN = EngineRegistry.EunoiaEngine_Camera_TN as TransformNode;
    if (!camTN) return;

    // Early exit if at rest and not looking
    const diffX = Math.abs(targetRotation.x - currentRotation.x);
    const diffY = Math.abs(targetRotation.y - currentRotation.y);
    if (!looking && diffX < 0.0001 && diffY < 0.0001) return;

    const deltaSec = ((EngineRegistry.EunoiaEngine_Engine as Engine)?.getDeltaTime() ?? 16.6) / 1000;
    const lerpFactor = 1 - Math.exp(-rotationDampingSpeed * deltaSec);

    currentRotation.x = Scalar.Lerp(currentRotation.x, targetRotation.x, lerpFactor);
    currentRotation.y = Scalar.Lerp(currentRotation.y, targetRotation.y, lerpFactor);

    camTN.rotation.x = currentRotation.x;
    camTN.rotation.y = currentRotation.y;
    if (EngineRegistry.EunoiaEngine_Camera) {
        (EngineRegistry.EunoiaEngine_Camera as FreeCamera).rotation.z = 0;
    }

    if (diffX >= 0.0001 || diffY >= 0.0001) {
        EngineRegistry.RequestRender?.(2);
    }
}

// Keyboard & Movement Inertia

const moves = {
    w: 0,
    a: 0,
    s: 0,
    d: 0,
    q: 0,
    e: 0
};

const moveSpeed = 6.0;
const moveDampingSpeed = 12;
const currentVelocity = { x: 0, y: 0, z: 0 };

function ViewportControlsUpdate() {
    const camTN = EngineRegistry.EunoiaEngine_Camera_TN as TransformNode;
    if (!camTN) return;

    const targetZ = (moves.w - moves.s) * moveSpeed;
    const targetX = (moves.d - moves.a) * moveSpeed;
    const targetY = (moves.e - moves.q) * moveSpeed;

    const isInputActive = moves.w || moves.a || moves.s || moves.d || moves.q || moves.e;
    const isMoving = Math.abs(currentVelocity.x) > 0.0001 || Math.abs(currentVelocity.y) > 0.0001 || Math.abs(currentVelocity.z) > 0.0001;

    // Early exit if completely stationary
    if (!isInputActive && !isMoving) return;

    const deltaSec = ((EngineRegistry.EunoiaEngine_Engine as Engine)?.getDeltaTime() ?? 16.6) / 1000;
    const lerpFactor = 1 - Math.exp(-moveDampingSpeed * deltaSec);

    currentVelocity.z = Scalar.Lerp(currentVelocity.z, targetZ, lerpFactor);
    currentVelocity.x = Scalar.Lerp(currentVelocity.x, targetX, lerpFactor);
    currentVelocity.y = Scalar.Lerp(currentVelocity.y, targetY, lerpFactor);

    if (Math.abs(currentVelocity.z) > 0.0001) {
        camTN.translate(_vecForward, currentVelocity.z * deltaSec, Space.LOCAL);
    }
    if (Math.abs(currentVelocity.x) > 0.0001) {
        camTN.translate(_vecRight, currentVelocity.x * deltaSec, Space.LOCAL);
    }
    if (Math.abs(currentVelocity.y) > 0.0001) {
        camTN.translate(_vecUp, currentVelocity.y * deltaSec, Space.WORLD);
    }

    EngineRegistry.RequestRender?.(2);
}

function KeyDown(e: KeyboardEvent) {
    if (e.key === 'w' || e.key === 'W') moves.w = 1;
    if (e.key === 'a' || e.key === 'A') moves.a = 1;
    if (e.key === 's' || e.key === 'S') moves.s = 1;
    if (e.key === 'd' || e.key === 'D') moves.d = 1;
    if (e.key === 'q' || e.key === 'Q') moves.q = 1;
    if (e.key === 'e' || e.key === 'E') moves.e = 1;
    EngineRegistry.RequestRender?.(15);
}

function KeyUp(e: KeyboardEvent) {
    if (e.key === 'w' || e.key === 'W') moves.w = 0;
    if (e.key === 'a' || e.key === 'A') moves.a = 0;
    if (e.key === 's' || e.key === 'S') moves.s = 0;
    if (e.key === 'd' || e.key === 'D') moves.d = 0;
    if (e.key === 'q' || e.key === 'Q') moves.q = 0;
    if (e.key === 'e' || e.key === 'E') moves.e = 0;
    EngineRegistry.RequestRender?.(15);
}