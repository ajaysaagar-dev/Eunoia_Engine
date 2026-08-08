import { Color3, Texture } from "@babylonjs/core";

export interface TexturesPath_Interface {
    albedoTexture?: string;
    metallicTexture?: string;
    emissiveTexture?: string;
    bumpTexture?: string;
    ambientTexture?: string;
    opacityTexture?: string;
    reflectivityTexture?: string;
    reflectanceTexture?: string;
    reflectionTexture?: string;
}

export interface TexturesOut_Interface {
    albedoTexture_Out: Texture | null;
    metallicTexture_Out: Texture | null;
    emissiveTexture_Out: Texture | null;
    bumpTexture_Out: Texture | null;
    ambientTexture_Out: Texture | null;
    opacityTexture_Out: Texture | null;
    reflectivityTexture_Out: Texture | null;
    reflectanceTexture_Out: Texture | null;
    reflectionTexture_Out: Texture | null;
}

export interface TextureLevels_Interface {
    albedoTexture?: number;
    metallicTexture?: number;
    emissiveTexture?: number;
    bumpTexture?: number;
    ambientTexture?: number;
    opacityTexture?: number;
    reflectivityTexture?: number;
    reflectanceTexture?: number;
    reflectionTexture?: number;
}

export interface TextureUVScale_Interface {
    u?: number;
    v?: number;
}

export interface MaterialColors_Interface {
    albedoColor?: Color3;
    emissiveColor?: Color3;
    ambientColor?: Color3;
    reflectivityColor?: Color3;
    reflectionColor?: Color3;
}

export interface MaterialValues_Interface {
    metallic?: number;
    roughness?: number;
    alpha?: number;
    directIntensity?: number;
    environmentIntensity?: number;
    emissiveIntensity?: number;
    specularIntensity?: number;
    useAlphaFromAlbedoTexture?: boolean;
    backFaceCulling?: boolean;
    zOffset?: number;
}