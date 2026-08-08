import { AreaLight, DirectionalLight, HemisphericLight, PointLight, RectAreaLight, Scene, SpotLight, Vector3 } from "@babylonjs/core";

const EunoiaEngine_Light = {
    CreatePointLight: async (name: string, position: Vector3) => new PointLight(name, position, ((window as any).EunoiaEngine_Scene as Scene)),
    CreateDirectionalLight: async (name: string, direction: Vector3) => new DirectionalLight(name, direction, ((window as any).EunoiaEngine_Scene as Scene)),
    CreateSpotLight: async (name: string, position: Vector3, direction: Vector3, angle: number, exponent: number) => new SpotLight(name, position, direction, angle, exponent, ((window as any).EunoiaEngine_Scene as Scene)),
    CreateRectAreaLight: async (name: string, position: Vector3, width: number, height: number) => new RectAreaLight(name, position, width, height, ((window as any).EunoiaEngine_Scene as Scene)),
    CreateHemisphericLight: async (name: string, direction: Vector3) => new HemisphericLight(name, direction, ((window as any).EunoiaEngine_Scene as Scene))
}

export default EunoiaEngine_Light;