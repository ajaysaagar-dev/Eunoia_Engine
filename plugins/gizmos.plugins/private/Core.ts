import {
    Scene,
    AbstractMesh,
    GizmoManager,
    Color4,
    PointerEventTypes,
    Nullable
} from "@babylonjs/core";
import EngineRegistry from "../../../engine/registry.plugins";
import EngineStates from "../../../engine/states.engine";

export type GizmoMode = "select" | "position" | "rotation" | "scale";

let scene: Scene | null = null;
let gizmoManager: GizmoManager | null = null;
let selectedMesh: Nullable<AbstractMesh> = null;
let activeGizmoMode: GizmoMode = "position";
let keyListenerAttached = false;

const SHARP_EDGE_COLOR = new Color4(0.23, 0.51, 0.96, 1.0); // Sharp crisp blue (#3b82f6)

export function InitGizmosPlugin(): void {
    scene = EngineRegistry.EunoiaEngine_Scene as Scene;
    if (!scene) return;

    if (gizmoManager) {
        gizmoManager.dispose();
        gizmoManager = null;
    }

    // Instantiate GizmoManager
    gizmoManager = new GizmoManager(scene);
    gizmoManager.usePointerToAttachGizmos = false; // We handle selection explicitly to ignore grid & gizmos

    // Setup pointer picking selection
    scene.onPointerObservable.add((pointerInfo) => {
        if (EngineStates.Mode !== "Editor") return;
        if (pointerInfo.type === PointerEventTypes.POINTERDOWN) {
            // Left-click selection (button 0)
            if (pointerInfo.event.button === 0) {
                const pickResult = pointerInfo.pickInfo;
                if (pickResult && pickResult.hit && pickResult.pickedMesh) {
                    const mesh = pickResult.pickedMesh;
                    // Check if clicked mesh is a valid scene object or gizmo/grid utility mesh
                    const isGizmoMesh = mesh.name.includes("gizmo") || mesh.name.includes("Gizmo") || mesh.name.includes("BoundingBox");
                    if (mesh.name === "EditorGridMesh") {
                        SelectMesh(null);
                    } else if (!isGizmoMesh) {
                        SelectMesh(mesh);
                    }
                } else {
                    SelectMesh(null);
                }
            }
        }
    });

    // Request rendering while gizmos are dragged
    gizmoManager.onAttachedToMeshObservable.add(() => {
        EngineRegistry.RequestRender?.(5);
    });

    // Attach Keyboard Shortcuts (Q, W, E, R)
    if (!keyListenerAttached) {
        window.addEventListener("keydown", handleKeyDown);
        keyListenerAttached = true;
    }

    // Register registry helpers
    EngineRegistry.SelectNode = (node: any) => SelectMesh(node);
    EngineRegistry.GetSelectedNode = () => selectedMesh;
    EngineRegistry.SetGizmoMode = (mode: GizmoMode) => SetGizmoMode(mode);
    EngineRegistry.GetGizmoMode = () => activeGizmoMode;

    SetGizmoMode("position");
}

function handleKeyDown(e: KeyboardEvent) {
    if (EngineStates.Mode !== "Editor") return;

    // Ignore gizmo shortcut keys if Right Mouse Button is held down (camera navigation)
    if (EngineRegistry.IsRightClickLooking?.() || (e.buttons & 2)) {
        return;
    }

    // Ignore keyboard shortcuts if user is typing in an input field
    const activeEl = document.activeElement;
    if (activeEl && (activeEl.tagName === "INPUT" || activeEl.tagName === "TEXTAREA" || activeEl.isContentEditable)) {
        return;
    }

    const key = e.key.toLowerCase();
    if (key === "q") {
        SetGizmoMode("select");
    } else if (key === "w") {
        SetGizmoMode("position");
    } else if (key === "e") {
        SetGizmoMode("rotation");
    } else if (key === "r") {
        SetGizmoMode("scale");
    }
}

export function SetGizmoMode(mode: GizmoMode): void {
    activeGizmoMode = mode;
    if (!gizmoManager) return;

    // Reset all transform gizmos first
    gizmoManager.positionGizmoEnabled = false;
    gizmoManager.rotationGizmoEnabled = false;
    gizmoManager.scaleGizmoEnabled = false;
    gizmoManager.boundingBoxGizmoEnabled = false;

    if (EngineStates.Mode === "Editor" && selectedMesh) {
        switch (mode) {
            case "position":
                gizmoManager.positionGizmoEnabled = true;
                if (gizmoManager.gizmos.positionGizmo) {
                    gizmoManager.gizmos.positionGizmo.onDragStartObservable.addOnce(() => EngineRegistry.RequestRender?.(60));
                }
                break;
            case "rotation":
                gizmoManager.rotationGizmoEnabled = true;
                if (gizmoManager.gizmos.rotationGizmo) {
                    // Disable matching mesh rotation for non-uniform scaling to prevent shear matrix warnings/errors
                    gizmoManager.gizmos.rotationGizmo.updateGizmoRotationToMatchAttachedMesh = false;
                    gizmoManager.gizmos.rotationGizmo.onDragStartObservable.addOnce(() => EngineRegistry.RequestRender?.(60));
                }
                break;
            case "scale":
                gizmoManager.scaleGizmoEnabled = true;
                if (gizmoManager.gizmos.scaleGizmo) {
                    gizmoManager.gizmos.scaleGizmo.onDragStartObservable.addOnce(() => EngineRegistry.RequestRender?.(60));
                }
                break;
            case "select":
                // No gizmos enabled
                break;
        }
    }

    EngineRegistry.RequestRender?.(10);
}

export function SelectMesh(mesh: Nullable<AbstractMesh>): void {
    // Disable sharp edges on previously selected mesh
    if (selectedMesh) {
        if (typeof (selectedMesh as any).disableEdgesRendering === "function") {
            (selectedMesh as any).disableEdgesRendering();
        }
    }

    selectedMesh = mesh;

    if (!gizmoManager) return;

    if (selectedMesh && EngineStates.Mode === "Editor") {
        gizmoManager.attachToMesh(selectedMesh);
        // Enable sharp geometric edge rendering on newly selected mesh
        if (typeof (selectedMesh as any).enableEdgesRendering === "function") {
            (selectedMesh as any).enableEdgesRendering(0.95);
            (selectedMesh as any).edgesWidth = 4.0;
            (selectedMesh as any).edgesColor = SHARP_EDGE_COLOR;
        }
        SetGizmoMode(activeGizmoMode);
    } else {
        gizmoManager.attachToMesh(null);
        gizmoManager.positionGizmoEnabled = false;
        gizmoManager.rotationGizmoEnabled = false;
        gizmoManager.scaleGizmoEnabled = false;
    }

    // Notify hierarchy / editor UI of selection change
    if (EngineRegistry.OnSelectedNodeChanged) {
        EngineRegistry.OnSelectedNodeChanged(selectedMesh);
    }

    EngineRegistry.RequestRender?.(10);
}

export function UpdateGizmoState(): void {
    const isEditor = EngineStates.Mode === "Editor";
    if (!isEditor && gizmoManager) {
        gizmoManager.attachToMesh(null);
        gizmoManager.positionGizmoEnabled = false;
        gizmoManager.rotationGizmoEnabled = false;
        gizmoManager.scaleGizmoEnabled = false;
        if (selectedMesh && typeof (selectedMesh as any).disableEdgesRendering === "function") {
            (selectedMesh as any).disableEdgesRendering();
        }
    } else if (isEditor && selectedMesh) {
        SelectMesh(selectedMesh);
    }
}

const EunoiaEngine_Gizmos = {
    Init: InitGizmosPlugin,
    SelectMesh,
    SetGizmoMode,
    UpdateState: UpdateGizmoState,
    GetSelectedMesh: () => selectedMesh,
    GetGizmoManager: () => gizmoManager
};

export default EunoiaEngine_Gizmos;
