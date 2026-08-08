import { Scene, SceneSerializer, Vector3, MeshBuilder, HemisphericLight } from "@babylonjs/core";
import EngineRegistry from "../../../engine/registry.plugins";
import EunoiaEngine_FileSystem from "../../file-system.plugins";

export function SerializeCurrentScene(): any {
    const scene = EngineRegistry.EunoiaEngine_Scene as Scene;
    if (!scene) return null;
    try {
        const serialized = SceneSerializer.Serialize(scene);
        const activePath = EngineRegistry.EunoiaEngine_ActiveScenePath || `${EngineRegistry.EunoiaEngine_ProjectPath || "C:\\Users\\Windows\\Documents\\Eunoia Projects"}\\MainLevel.scene.json`;
        const sceneId = EngineRegistry.GetOrCreateResourceId?.(activePath);
        if (sceneId) {
            serialized.metadata = {
                ...(serialized.metadata || {}),
                sceneId
            };
        }
        return serialized;
    } catch (e) {
        console.warn("EUNOIAENGINE > SceneSerializer warning:", e);
        return {
            producer: { name: "Eunoia Engine", version: "1.0" },
            autoClear: true,
            clearColor: [0.04, 0.06, 0.1, 1],
            ambientColor: [0, 0, 0],
            gravity: [0, -9.81, 0]
        };
    }
}

export function SaveActiveScene(filePath?: string): boolean {
    const activePath = filePath || EngineRegistry.EunoiaEngine_ActiveScenePath || `${EngineRegistry.EunoiaEngine_ProjectPath || "C:\\Users\\Windows\\Documents\\Eunoia Projects"}\\MainLevel.scene.json`;
    const serialized = SerializeCurrentScene();
    if (!serialized) return false;

    try {
        if (typeof window !== 'undefined' && EunoiaEngine_FileSystem && EunoiaEngine_FileSystem.writeFileSync) {
            EunoiaEngine_FileSystem.writeFileSync(activePath, JSON.stringify(serialized, null, 2), 'utf-8');
            EngineRegistry.EunoiaEngine_ActiveScenePath = activePath;

            // Sync with Reference System
            const sceneId = EngineRegistry.GetOrCreateResourceId?.(activePath);
            if (sceneId) {
                EngineRegistry.EunoiaEngine_ActiveSceneId = sceneId;
            }

            if (typeof localStorage !== 'undefined') {
                localStorage.setItem("EunoiaEngine_LastActiveScene", activePath);
            }
            console.log("EUNOIAENGINE > Scene saved successfully to:", activePath, "with ID:", sceneId);
            return true;
        }
    } catch (err) {
        console.error("EUNOIAENGINE > Could not save scene to file:", err);
    }
    return false;
}

export function CreateNewSceneFile(fileName: string, parentPath: string): string | null {
    let cleanName = fileName.trim();
    if (cleanName.toLowerCase().endsWith(".scene.json")) {
        // already has correct extension
    } else if (cleanName.toLowerCase().endsWith(".scene")) {
        cleanName = `${cleanName}.json`;
    } else if (cleanName.toLowerCase().endsWith(".json")) {
        cleanName = cleanName.substring(0, cleanName.length - 5) + ".scene.json";
    } else {
        cleanName = `${cleanName}.scene.json`;
    }

    const fullPath = `${parentPath}\\${cleanName}`;
    const sceneId = EngineRegistry.GetOrCreateResourceId?.(fullPath);

    const defaultBabylonSceneData = {
        "producer": {
            "name": "Eunoia Engine",
            "version": "1.0.0"
        },
        "metadata": {
            "sceneId": sceneId || 0
        },
        "autoClear": true,
        "clearColor": [0.043, 0.058, 0.098, 1],
        "gravity": [0, -9.81, 0],
        "materials": [],
        "cameras": [],
        "lights": [
            {
                "name": "SunLight",
                "id": "SunLight",
                "type": 3,
                "direction": [0, 1, 0],
                "intensity": 0.8
            }
        ],
        "meshes": [
            {
                "name": "DefaultCube",
                "id": "DefaultCube",
                "type": "Mesh",
                "position": [0, 1, 0],
                "rotation": [0, 0, 0],
                "scaling": [1, 1, 1],
                "isVisible": true,
                "isEnabled": true
            }
        ]
    };

    try {
        if (typeof window !== 'undefined' && EunoiaEngine_FileSystem && EunoiaEngine_FileSystem.writeFileSync) {
            EunoiaEngine_FileSystem.writeFileSync(fullPath, JSON.stringify(defaultBabylonSceneData, null, 2), 'utf-8');
            EngineRegistry.GetOrCreateResourceId?.(fullPath);
            return fullPath;
        }
    } catch (err) {
        console.warn("EUNOIAENGINE > Could not create scene file:", err);
    }
    return null;
}

export function ClearSceneUserObjects(): void {
    const scene = EngineRegistry.EunoiaEngine_Scene as Scene;
    if (!scene) return;

    // Deselect active node & clear gizmos
    EngineRegistry.SelectNode?.(null);

    // Dispose all user meshes (excluding editor grid and camera)
    const meshes = [...scene.meshes];
    meshes.forEach((mesh) => {
        const name = mesh.name;
        if (
            name !== "EditorGridMesh" &&
            name !== "Viewport_Camera_Parent" &&
            name !== "viewport_camera" &&
            !name.includes("gizmo") &&
            !name.includes("Gizmo") &&
            !name.includes("BoundingBox")
        ) {
            mesh.dispose();
        }
    });

    // Dispose user lights
    const lights = [...scene.lights];
    lights.forEach((light) => {
        if (!light.name.includes("gizmo")) {
            light.dispose();
        }
    });
}

export async function LoadSceneFromFile(targetPath: string): Promise<boolean> {
    const scene = EngineRegistry.EunoiaEngine_Scene as Scene;
    if (!scene) return false;

    try {
        if (typeof window !== 'undefined' && EunoiaEngine_FileSystem && EunoiaEngine_FileSystem.existsSync) {
            if (!EunoiaEngine_FileSystem.existsSync(targetPath)) {
                return false;
            }

            // 1. Save currently active scene first
            if (EngineRegistry.EunoiaEngine_ActiveScenePath && EngineRegistry.EunoiaEngine_ActiveScenePath !== targetPath) {
                SaveActiveScene(EngineRegistry.EunoiaEngine_ActiveScenePath);
            }

            // 2. Dispose current user objects
            ClearSceneUserObjects();

            // 3. Read target scene file content
            const fileData = EunoiaEngine_FileSystem.readFileSync(targetPath, 'utf-8');
            const sceneJson = JSON.parse(fileData);

            // Recreate scene objects from JSON data
            if (sceneJson.meshes && Array.isArray(sceneJson.meshes)) {
                sceneJson.meshes.forEach((m: any) => {
                    if (m.name !== "EditorGridMesh") {
                        const box = MeshBuilder.CreateBox(m.name || "Mesh", { size: 2 }, scene);
                        if (m.position) box.position = new Vector3(m.position[0], m.position[1], m.position[2]);
                        if (m.rotation) box.rotation = new Vector3(m.rotation[0], m.rotation[1], m.rotation[2]);
                        if (m.scaling) box.scaling = new Vector3(m.scaling[0], m.scaling[1], m.scaling[2]);
                    }
                });
            }

            if (sceneJson.lights && Array.isArray(sceneJson.lights)) {
                sceneJson.lights.forEach((l: any) => {
                    const light = new HemisphericLight(l.name || "Light", new Vector3(0, 1, 0), scene);
                    if (l.intensity) light.intensity = l.intensity;
                });
            }

            EngineRegistry.EunoiaEngine_ActiveScenePath = targetPath;
            const sceneId = EngineRegistry.GetOrCreateResourceId?.(targetPath);
            if (sceneId) {
                EngineRegistry.EunoiaEngine_ActiveSceneId = sceneId;
            }

            if (typeof localStorage !== 'undefined') {
                localStorage.setItem("EunoiaEngine_LastActiveScene", targetPath);
            }

            EngineRegistry.RequestRender?.(15);
            return true;
        }
    } catch (err) {
        console.error("EUNOIAENGINE > Could not load scene from file:", err);
    }
    return false;
}

export function GetLastOpenedScenePath(): string | null {
    try {
        if (typeof localStorage !== 'undefined') {
            const lastPath = localStorage.getItem("EunoiaEngine_LastActiveScene");
            if (lastPath && EunoiaEngine_FileSystem && EunoiaEngine_FileSystem.existsSync) {
                if (EunoiaEngine_FileSystem.existsSync(lastPath)) {
                    return lastPath;
                }
            }
        }
    } catch (e) { }
    return null;
}
