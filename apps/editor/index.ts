import { Color3, CreateBox, CreateGround, PointLight, Vector3 } from "@babylonjs/core";
import EunoiaEngine_Camera from "../../plugins/camera";
import EunoiaEngine_Engine from "../../plugins/engine";
import EunoiaEngine_Renderer from "../../plugins/renderer";
import EunoiaEngine_Scene from "../../plugins/scene";
import EunoiaEngine_Stats from "../../plugins/stats";
import EunoiaEngine_Meshes from "../../plugins/meshes";
import EunoiaEngine_Light from "../../plugins/lights";
import EunoiaEngine_Shadows from "../../plugins/shadows";
import EunoiaEngine_Materials from "../../plugins/materials";


async function start() {

    // Base ---------------------------->
    await EunoiaEngine_Engine();
    await EunoiaEngine_Scene();
    await EunoiaEngine_Camera();
    await EunoiaEngine_Stats.ShowFPS();

    // Test ---------------------------->
    const cube = await EunoiaEngine_Meshes.CreateBox('Name 01', { size: 2 });
    cube.position.y = 1;
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

    //Renderer ------------------------>
    await EunoiaEngine_Renderer();

}

start();