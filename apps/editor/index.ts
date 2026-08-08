import { Color3, PointLight, Vector3 } from "@babylonjs/core";
import EunoiaEngine_Camera from "../../plugins/camera.plugins";
import EunoiaEngine_Engine from "../../plugins/engine.plugins";
import EunoiaEngine_Renderer, { Renderer } from "../../plugins/renderer.plugins";
import EunoiaEngine_Scene, { GetLastOpenedScenePath, LoadSceneFromFile } from "../../plugins/scene.plugins";
import EunoiaEngine_Stats from "../../plugins/stats.plugins";
import EunoiaEngine_Meshes from "../../plugins/meshes.plugins";
import EunoiaEngine_Light from "../../plugins/lights.plugins";
import EunoiaEngine_Shadows from "../../plugins/shadows.plugins";
import EunoiaEngine_Materials from "../../plugins/materials.plugins";
import EunoiaEngine_EditorTools from "../../plugins/editor-tools.plugins";
import EunoiaEngine_Gizmos from "../../plugins/gizmos.plugins";
import EunoiaEngine_References from "../../plugins/references.plugins";
import EngineRegistry from "../../engine/registry.plugins";
import {
    setupModeToggle,
    initHierarchyUI,
    updateHierarchyUI,
    initAssetBrowserUI,
    initPropertiesUI,
    updateTopSceneNameUI
} from "./scripts";


async function start() {

    // Base ---------------------------->
    await EunoiaEngine_Engine();
    await EunoiaEngine_Scene();
    await EunoiaEngine_Camera();
    await EunoiaEngine_Stats.ShowFPS();
    await EunoiaEngine_EditorTools.Init();
    await EunoiaEngine_Gizmos.Init();
    EunoiaEngine_References.Init();

    // Init Editor UI Components
    initHierarchyUI();
    initAssetBrowserUI();
    initPropertiesUI();

    const resetEditorState = () => {
        // Restore viewport camera initial transform & physics state
        (EngineRegistry.ResetViewportCamera as any)?.();

        // Refresh grid & gizmo visibility
        EunoiaEngine_EditorTools.UpdateState();
        EunoiaEngine_Gizmos.UpdateState();

        // Update Scene Hierarchy Panel & Top Scene Badge
        updateHierarchyUI();
        updateTopSceneNameUI();

        // Request initial render frame burst to display refreshed scene
        EngineRegistry.RequestRender?.(15);
    };

    // Setup Editor Frontend UI Scripts
    setupModeToggle(resetEditorState);

    // Check if a last opened scene exists to restore upon project reload
    const lastOpenedScenePath = GetLastOpenedScenePath();
    if (lastOpenedScenePath) {
        await LoadSceneFromFile(lastOpenedScenePath);
    } else {
        // Setup initial default scene objects
        const cube = await EunoiaEngine_Meshes.CreateBox('Name 01', { size: 2 });
        cube.position.y = 1;

        class CubeRotate extends Renderer {
            public GameStart(): void | Promise<void> {
                cube.position.y = 6;
            }

            public GameUpdate(): void {
                cube.rotation.y += 0.01;
            }
        }; new CubeRotate();

        const light = await EunoiaEngine_Light.CreatePointLight('name', new Vector3(4, 4, -4));
        light.position.set(4, 8, -4);
        light.intensity = 25;
        const ground = await EunoiaEngine_Meshes.CreateGround('Name 03', { width: 10, height: 10 });

        const aLight = await EunoiaEngine_Light.CreateHemisphericLight('light', new Vector3(0, 1, 0));
        aLight.intensity = 0.01;

        const mat = await EunoiaEngine_Materials.PBRMaterial('name', {}, {}, {}, { albedoColor: Color3.Blue() });

        const mat2 = await EunoiaEngine_Materials.PBRMaterial('mat 01', {
            albedoTexture: 'https://dl.polyhaven.org/file/ph-assets/Textures/jpg/1k/forest_ground_06/forest_ground_06_diff_1k.jpg',
            bumpTexture: 'https://dl.polyhaven.org/file/ph-assets/Textures/jpg/1k/forest_ground_06/forest_ground_06_nor_gl_1k.jpg'
        },
            {
                albedoTexture: 1,
                bumpTexture: 10,
            },
            {
                u: 4,
                v: 4
            }, {}, { roughness: 1 }
        );

        cube.material = mat;
        ground.material = mat2;

        const SG = EunoiaEngine_Shadows.ShadowGenerator(light, 1024);
        EunoiaEngine_Shadows.ShadowRecieveEnable(ground);
        EunoiaEngine_Shadows.ShadowEnable(cube);

        // Select default cube in editor
        EunoiaEngine_Gizmos.SelectMesh(cube);
    }

    updateHierarchyUI();
    updateTopSceneNameUI();

    // Renderer ------------------------>
    await EunoiaEngine_Renderer();

    EngineRegistry.RequestRender?.(10);
}

start();