import { CreateBox, CreateGround, Scene } from "@babylonjs/core";
import { BoxMeshInterface, GroundMeshInterface } from "../../../types/meshes.types";
import EngineRegistry from "../../../engine/registry.plugins";

const EunoiaEngine_Meshes = {
    CreateBox: async (name: string, options: BoxMeshInterface) =>
        (CreateBox(name, options, (EngineRegistry.EunoiaEngine_Scene as Scene))),
    CreateGround: async (name: string, options: GroundMeshInterface) =>
        (CreateGround(name, options, (EngineRegistry.EunoiaEngine_Scene as Scene)))
}

export default EunoiaEngine_Meshes;