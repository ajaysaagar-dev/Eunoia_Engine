import { AreaLight, DirectionalLight, HemisphericLight, PointLight, RectAreaLight, Scene, SpotLight, Vector3 } from "@babylonjs/core";
import EngineRegistry from "../../registry.plugins";

const EunoiaEngine_Light = {
    CreatePointLight: async (name: string, position: Vector3) => new PointLight(name, position, (EngineRegistry.EunoiaEngine_Scene as Scene)),
    CreateDirectionalLight: async (name: string, direction: Vector3) => new DirectionalLight(name, direction, (EngineRegistry.EunoiaEngine_Scene as Scene)),
    CreateSpotLight: async (name: string, position: Vector3, direction: Vector3, angle: number, exponent: number) => new SpotLight(name, position, direction, angle, exponent, (EngineRegistry.EunoiaEngine_Scene as Scene)),
    CreateRectAreaLight: async (name: string, position: Vector3, width: number, height: number) => new RectAreaLight(name, position, width, height, (EngineRegistry.EunoiaEngine_Scene as Scene)),
    CreateHemisphericLight: async (name: string, direction: Vector3) => new HemisphericLight(name, direction, (EngineRegistry.EunoiaEngine_Scene as Scene))
}

export default EunoiaEngine_Light;