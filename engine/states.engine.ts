import EngineRegistry from "./registry.plugins";

export interface States {
    Mode: 'Editor' | 'Game';
}

const EngineStates: States = {
    Mode: 'Editor'
};

EngineRegistry.EunoiaEngine_States = EngineStates;

export default EngineStates;