import { Color3, CreateGround, Mesh, Scene } from "@babylonjs/core";
import { GridMaterial } from "@babylonjs/materials";
import EngineRegistry from "../../../engine/registry.plugins";
import EngineStates from "../../../engine/states.engine";

let gridMesh: Mesh | null = null;
let gridMaterial: GridMaterial | null = null;

export async function InitEditorTools(options?: { size?: number; opacity?: number }) {
    const scene = EngineRegistry.EunoiaEngine_Scene as Scene;
    if (!scene) return;

    if (gridMesh) {
        gridMesh.dispose();
        gridMesh = null;
    }

    const gridOpacity = options?.opacity ?? 0.15;
    const gridSize = options?.size ?? 200;

    gridMaterial = new GridMaterial("EditorGridMaterial", scene);
    gridMaterial.majorUnitFrequency = 5;
    gridMaterial.minorUnitVisibility = 0.35;
    gridMaterial.gridRatio = 1;
    gridMaterial.opacity = gridOpacity;
    gridMaterial.backFaceCulling = false;
    gridMaterial.mainColor = new Color3(0.15, 0.15, 0.15);
    gridMaterial.lineColor = new Color3(0.7, 0.7, 0.7);

    // Single subdivision quad for maximum culling and minimal vertex overhead
    gridMesh = CreateGround("EditorGridMesh", { width: gridSize, height: gridSize, subdivisions: 1 }, scene);
    gridMesh.material = gridMaterial;
    gridMesh.isPickable = false;
    gridMesh.position.y = 0.001;

    UpdateGridState();
}

export function UpdateGridState() {
    if (!gridMesh) return;
    const mode = EngineStates.Mode ?? 'Editor';
    const isEditor = mode === 'Editor';
    if (gridMesh.isEnabled() !== isEditor) {
        gridMesh.setEnabled(isEditor);
        EngineRegistry.RequestRender?.(5);
    }
}

const EunoiaEngine_EditorTools = {
    Init: InitEditorTools,
    UpdateState: UpdateGridState,
    SetGridOpacity: (opacity: number) => {
        if (gridMaterial) {
            gridMaterial.opacity = opacity;
            EngineRegistry.RequestRender?.(5);
        }
    },
    GetGridMesh: () => gridMesh,
    GetGridMaterial: () => gridMaterial
};

export default EunoiaEngine_EditorTools;
