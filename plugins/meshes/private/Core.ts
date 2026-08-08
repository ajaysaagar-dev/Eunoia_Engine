import { CreateBox, CreateGround, Scene } from "@babylonjs/core";
import { BoxMeshInterface, GroundMeshInterface } from "../../../types/meshes.types";

const EunoiaEngine_Meshes = {
    CreateBox: async (name: string, options: BoxMeshInterface) =>
        (CreateBox(name, options, ((window as any).EunoiaEngine_Scene as Scene))),
    CreateGround: async (name: string, options: GroundMeshInterface) =>
        (CreateGround(name, options, ((window as any).EunoiaEngine_Scene as Scene)))
}

export default EunoiaEngine_Meshes;