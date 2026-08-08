import { AbstractMesh, Node, Quaternion, Scene, TransformNode, Vector3 } from "@babylonjs/core";
import EngineRegistry from "../../../engine/registry.plugins";

export interface NodeTransformSnapshot {
    uniqueId: number;
    name: string;
    position: Vector3;
    rotation: Vector3;
    rotationQuaternion: Quaternion | null;
    scaling: Vector3;
    isEnabled: boolean;
    visibility?: number;
    parentUniqueId?: number | null;
}

export interface SceneSnapshot {
    nodeSnapshots: Map<number, NodeTransformSnapshot>;
}

let currentSceneSnapshot: SceneSnapshot | null = null;

export function TakeSceneSnapshot() {
    const scene = EngineRegistry.EunoiaEngine_Scene as Scene;
    if (!scene) return;

    const nodeSnapshots = new Map<number, NodeTransformSnapshot>();
    const nodes = scene.getNodes();
    const len = nodes.length;

    for (let i = 0; i < len; i++) {
        const node = nodes[i];

        // Skip editor auxiliary objects like grid or camera parent
        if (node.name === "EditorGridMesh" || node.name === "Viewport_Camera_Parent" || node.name === "viewport_camera") continue;

        const snapshot: NodeTransformSnapshot = {
            uniqueId: node.uniqueId,
            name: node.name,
            position: Vector3.Zero(),
            rotation: Vector3.Zero(),
            rotationQuaternion: null,
            scaling: Vector3.One(),
            isEnabled: node.isEnabled(),
            parentUniqueId: node.parent ? node.parent.uniqueId : null
        };

        if (node instanceof TransformNode) {
            snapshot.position = node.position.clone();
            snapshot.rotation = node.rotation.clone();
            if (node.rotationQuaternion) {
                snapshot.rotationQuaternion = node.rotationQuaternion.clone();
            }
            snapshot.scaling = node.scaling.clone();
        }

        if (node instanceof AbstractMesh) {
            snapshot.visibility = node.visibility;
        }

        nodeSnapshots.set(node.uniqueId, snapshot);
    }

    currentSceneSnapshot = { nodeSnapshots };
}

export function RestoreSceneSnapshot() {
    const scene = EngineRegistry.EunoiaEngine_Scene as Scene;
    if (!scene || !currentSceneSnapshot) return;

    const { nodeSnapshots } = currentSceneSnapshot;
    const nodes = scene.getNodes();
    const len = nodes.length;

    for (let i = 0; i < len; i++) {
        const node = nodes[i];
        const snapshot = nodeSnapshots.get(node.uniqueId);

        if (!snapshot) continue;

        // Restore enabled state
        if (node.isEnabled() !== snapshot.isEnabled) {
            node.setEnabled(snapshot.isEnabled);
        }

        // Restore transforms
        if (node instanceof TransformNode) {
            node.position.copyFrom(snapshot.position);
            node.rotation.copyFrom(snapshot.rotation);
            if (snapshot.rotationQuaternion) {
                if (!node.rotationQuaternion) {
                    node.rotationQuaternion = snapshot.rotationQuaternion.clone();
                } else {
                    node.rotationQuaternion.copyFrom(snapshot.rotationQuaternion);
                }
            } else {
                node.rotationQuaternion = null;
            }
            node.scaling.copyFrom(snapshot.scaling);
        }

        if (node instanceof AbstractMesh && snapshot.visibility !== undefined) {
            node.visibility = snapshot.visibility;
        }
    }
}

EngineRegistry.TakeSceneSnapshot = TakeSceneSnapshot;
EngineRegistry.RestoreSceneSnapshot = RestoreSceneSnapshot;
