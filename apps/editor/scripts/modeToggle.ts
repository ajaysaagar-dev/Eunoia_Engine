import EngineRegistry from "../../../engine/registry.plugins";
import EngineStates from "../../../engine/states.engine";
import EunoiaEngine_EditorTools from "../../../plugins/editor-tools.plugins";

export function setupModeToggle(onResetEditorState?: () => void) {
    const btnEditor = document.getElementById('btn-mode-editor');
    const btnGame = document.getElementById('btn-mode-game');

    const switchMode = (mode: 'Editor' | 'Game') => {
        const prevMode = EngineStates.Mode;
        if (prevMode === mode) return;

        if (mode === 'Game' && prevMode === 'Editor') {
            // Take native state snapshot before game mode starts
            EngineRegistry.TakeSceneSnapshot?.();
        }

        EngineStates.Mode = mode;

        if (btnEditor && btnGame) {
            if (mode === 'Editor') {
                btnEditor.classList.add('active');
                btnGame.classList.remove('active');
            } else {
                btnGame.classList.add('active');
                btnEditor.classList.remove('active');
            }
        }

        if (mode === 'Editor' && prevMode === 'Game') {
            // Restore native transforms & state of all scene objects
            EngineRegistry.RestoreSceneSnapshot?.();
            (EngineRegistry.ResetViewportCamera as any)?.();

            if (onResetEditorState) {
                onResetEditorState();
            }

            EunoiaEngine_EditorTools.UpdateState();
            EngineRegistry.RequestRender?.(15);
        } else {
            EunoiaEngine_EditorTools.UpdateState();
            EngineRegistry.RequestRender?.(15);
        }
    };

    btnEditor?.addEventListener('click', () => switchMode('Editor'));
    btnGame?.addEventListener('click', () => switchMode('Game'));
}
