import { Color4, Vector4 } from "@babylonjs/core";


export interface BoxMeshInterface {
    size?: number;
    width?: number;
    height?: number;
    depth?: number;
    faceUV?: Vector4[];
    faceColors?: Color4[];
    sideOrientation?: number;
    frontUVs?: Vector4;
    backUVs?: Vector4;
    wrap?: boolean;
    topBaseAt?: number;
    bottomBaseAt?: number;
    updatable?: boolean;
}

export interface GroundMeshInterface {
    width?: number;
    height?: number;
    subdivisions?: number;
    subdivisionsX?: number;
    subdivisionsY?: number;
    updatable?: boolean;
}