import { AbstractMesh, IShadowLight, Scene, ShadowGenerator } from "@babylonjs/core";
import EngineRegistry from "../../../engine/registry.plugins";

EngineRegistry.EunoiaEngine_ShadowGenerators ??= [];

export async function ShadowGenerator_(light: IShadowLight, resolution: 2 | 4 | 8 | 16 | 32 | 64 | 128 | 256 | 512 | 1024 | 2048 | 4096 = 512) {
    const SG = new ShadowGenerator(resolution, light);
    SG.id = `${Math.floor(Math.random() * 9999999999) + 1111111111}`;
    EngineRegistry.EunoiaEngine_ShadowGenerators ??= [];
    (EngineRegistry.EunoiaEngine_ShadowGenerators as ShadowGenerator[]).push(SG);
    return SG;
}

export async function ShadowEnable_(mesh: AbstractMesh) {
    (EngineRegistry.EunoiaEngine_ShadowGenerators as ShadowGenerator[])?.forEach((SG) => {
        SG.addShadowCaster(mesh);
    });
}

export async function ShadowDisable_(mesh: AbstractMesh) {
    (EngineRegistry.EunoiaEngine_ShadowGenerators as ShadowGenerator[])?.forEach((SG) => {
        SG.removeShadowCaster(mesh);
    });
}

export async function ShadowRecieveEnable_(mesh: AbstractMesh) {
    mesh.receiveShadows = true;
}

export async function ShadowRecieveDisable_(mesh: AbstractMesh) {
    mesh.receiveShadows = false;
}