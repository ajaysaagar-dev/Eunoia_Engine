import type { States } from "./states.engine";

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
    EunoiaEngine_States?: States;
    EunoiaEngine_DefaultProjectPath?: string;
    EunoiaEngine_ProjectPath?: string;
    EunoiaEngine_ActiveScenePath?: string;
    EunoiaEngine_ActiveSceneId?: number;
    RequestRender?: (frames?: number) => void;
    ResetViewportCamera?: () => void;
    TakeSceneSnapshot?: () => void;
    RestoreSceneSnapshot?: () => void;
    SelectNode?: (node: any) => void;
    GetSelectedNode?: () => any;
    SetGizmoMode?: (mode: "select" | "position" | "rotation" | "scale") => void;
    GetGizmoMode?: () => string;
    OnSelectedNodeChanged?: (node: any) => void;
    SaveActiveScene?: (filePath?: string) => boolean;
    GetOrCreateResourceId?: (pathStr: string) => number;
    ResolveResourceIdToPath?: (id: number) => string | null;
    ResolveResourceIdToAbsolutePath?: (id: number) => string | null;
    ResolveResourcePathToId?: (pathStr: string) => number | null;
    RegisterResource?: (pathStr: string) => any;
    UpdateResourcePath?: (oldPath: string, newPath: string) => boolean;
    RemoveResourceReference?: (idOrPath: number | string) => boolean;
    SaveResourceReferences?: () => void;
    [key: string]: any;
}

export const EngineRegistry: IEngineRegistry = {};
export default EngineRegistry;