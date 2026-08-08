import { PBRMaterial, Scene, Texture } from "@babylonjs/core";
import {
    MaterialColors_Interface,
    MaterialValues_Interface,
    TextureLevels_Interface,
    TexturesOut_Interface,
    TexturesPath_Interface,
    TextureUVScale_Interface
} from "./types";
import EngineRegistry from "../../../engine/registry.plugins";

EngineRegistry.EunoiaEngine_Materials ??= [];

export async function PBRMaterial_(
    name: string,
    textures: TexturesPath_Interface = {},
    levels: TextureLevels_Interface = {},
    uvScale: TextureUVScale_Interface = {},
    colors: MaterialColors_Interface = {},
    values: MaterialValues_Interface = {}
): Promise<PBRMaterial> {

    const scene = EngineRegistry.EunoiaEngine_Scene as Scene;
    const material = new PBRMaterial(name, scene);

    const loaded = await LoadTextures(textures);

    ApplyTextureSettings(loaded.albedoTexture_Out, levels.albedoTexture, uvScale);
    ApplyTextureSettings(loaded.metallicTexture_Out, levels.metallicTexture, uvScale);
    ApplyTextureSettings(loaded.emissiveTexture_Out, levels.emissiveTexture, uvScale);
    ApplyTextureSettings(loaded.bumpTexture_Out, levels.bumpTexture, uvScale);
    ApplyTextureSettings(loaded.ambientTexture_Out, levels.ambientTexture, uvScale);
    ApplyTextureSettings(loaded.opacityTexture_Out, levels.opacityTexture, uvScale);
    ApplyTextureSettings(loaded.reflectivityTexture_Out, levels.reflectivityTexture, uvScale);
    ApplyTextureSettings(loaded.reflectanceTexture_Out, levels.reflectanceTexture, uvScale);
    ApplyTextureSettings(loaded.reflectionTexture_Out, levels.reflectionTexture, uvScale);

    material.albedoTexture = loaded.albedoTexture_Out;
    material.metallicTexture = loaded.metallicTexture_Out;
    material.emissiveTexture = loaded.emissiveTexture_Out;
    material.bumpTexture = loaded.bumpTexture_Out;
    material.ambientTexture = loaded.ambientTexture_Out;
    material.opacityTexture = loaded.opacityTexture_Out;
    material.reflectivityTexture = loaded.reflectivityTexture_Out;
    material.reflectanceTexture = loaded.reflectanceTexture_Out;
    material.reflectionTexture = loaded.reflectionTexture_Out;

    if (colors.albedoColor) material.albedoColor = colors.albedoColor;
    if (colors.emissiveColor) material.emissiveColor = colors.emissiveColor;
    if (colors.ambientColor) material.ambientColor = colors.ambientColor;
    if (colors.reflectivityColor) material.reflectivityColor = colors.reflectivityColor;
    if (colors.reflectionColor) material.reflectionColor = colors.reflectionColor;

    ApplyMaterialValues(material, values);

    EngineRegistry.EunoiaEngine_Materials ??= [];
    (EngineRegistry.EunoiaEngine_Materials as PBRMaterial[]).push(material);

    return material;
}

function ApplyTextureSettings(
    texture: Texture | null,
    level?: number,
    uvScale: TextureUVScale_Interface = {}
): void {
    if (!texture) return;

    texture.level = level ?? 1;
    texture.uScale = uvScale.u ?? 1;
    texture.vScale = uvScale.v ?? 1;
}

function ApplyMaterialValues(
    material: PBRMaterial,
    values: MaterialValues_Interface = {}
): void {
    material.metallic = values.metallic ?? 0;
    material.roughness = values.roughness ?? 0.5;

    if (values.alpha !== undefined) material.alpha = values.alpha;
    if (values.directIntensity !== undefined) material.directIntensity = values.directIntensity;
    if (values.environmentIntensity !== undefined) material.environmentIntensity = values.environmentIntensity;
    if (values.emissiveIntensity !== undefined) material.emissiveIntensity = values.emissiveIntensity;
    if (values.specularIntensity !== undefined) material.specularIntensity = values.specularIntensity;
    if (values.useAlphaFromAlbedoTexture !== undefined) {
        material.useAlphaFromAlbedoTexture = values.useAlphaFromAlbedoTexture;
    }
    if (values.backFaceCulling !== undefined) {
        material.backFaceCulling = values.backFaceCulling;
    }
    if (values.zOffset !== undefined) {
        material.zOffset = values.zOffset;
    }
}

async function LoadTextures(
    textures: TexturesPath_Interface
): Promise<TexturesOut_Interface> {

    const [
        albedoTexture_Out,
        metallicTexture_Out,
        emissiveTexture_Out,
        bumpTexture_Out,
        ambientTexture_Out,
        opacityTexture_Out,
        reflectivityTexture_Out,
        reflectanceTexture_Out,
        reflectionTexture_Out
    ] = await Promise.all([
        LoadSingleTexture(textures.albedoTexture),
        LoadSingleTexture(textures.metallicTexture),
        LoadSingleTexture(textures.emissiveTexture),
        LoadSingleTexture(textures.bumpTexture),
        LoadSingleTexture(textures.ambientTexture),
        LoadSingleTexture(textures.opacityTexture),
        LoadSingleTexture(textures.reflectivityTexture),
        LoadSingleTexture(textures.reflectanceTexture),
        LoadSingleTexture(textures.reflectionTexture)
    ]);

    return {
        albedoTexture_Out,
        metallicTexture_Out,
        emissiveTexture_Out,
        bumpTexture_Out,
        ambientTexture_Out,
        opacityTexture_Out,
        reflectivityTexture_Out,
        reflectanceTexture_Out,
        reflectionTexture_Out
    };
}

async function LoadSingleTexture(path?: string): Promise<Texture | null> {
    if (!path) {
        return null;
    }

    const scene = EngineRegistry.EunoiaEngine_Scene as Scene;

    return new Promise<Texture>((resolve, reject) => {
        const texture = new Texture(
            path,
            scene,
            false,
            false,
            Texture.TRILINEAR_SAMPLINGMODE,
            () => resolve(texture),
            (message) => reject(new Error(message))
        );
    });
}