export interface IEngineRegistry {
    EunoiaEngine_Viewport?: HTMLCanvasElement | HTMLElement | null;
    EunoiaEngine_Engine?: any;
    EunoiaEngine_GraphicsAPI?: string | null;
    EunoiaEngine_Scene?: any;
    EunoiaEngine_Camera?: any;
    EunoiaEngine_Camera_TN?: any;
    EunoiaEngine_Renderer?: any;
    EunoiaEngine_Renderers?: any[];
    EunoiaEngine_Materials?: any[];
    EunoiaEngine_ShadowGenerators?: any[];
    [key: string]: any;
}

export const EngineRegistry: IEngineRegistry = {};
export default EngineRegistry;