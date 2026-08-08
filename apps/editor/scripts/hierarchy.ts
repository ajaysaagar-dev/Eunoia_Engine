import { AbstractMesh, Camera, Light, Node, Scene, TransformNode } from "@babylonjs/core";
import EngineRegistry from "../../../engine/registry.plugins";

let activeSelectedUniqueId: number | null = null;

export function initHierarchyUI() {
    const hierarchyTreeEl = document.getElementById("hierarchy-tree");
    const refreshBtn = document.getElementById("btn-refresh-hierarchy");

    if (!hierarchyTreeEl) return;

    refreshBtn?.addEventListener("click", () => {
        updateHierarchyUI();
        EngineRegistry.RequestRender?.(10);
    });

    // Sync node selection when picked in 3D viewport / gizmo
    EngineRegistry.OnSelectedNodeChanged = (node: any) => {
        activeSelectedUniqueId = node ? node.uniqueId : null;
        updateHierarchyUI();
    };

    updateHierarchyUI();
}

export function updateHierarchyUI() {
    const scene = EngineRegistry.EunoiaEngine_Scene as Scene;
    const hierarchyTreeEl = document.getElementById("hierarchy-tree");
    const countBadge = document.getElementById("hierarchy-node-count");

    if (!scene || !hierarchyTreeEl) return;

    const allNodes = scene.getNodes();

    // Filter internal editor auxiliary objects and gizmos
    const visibleNodes = allNodes.filter(
        node => node.name !== "EditorGridMesh" &&
            node.name !== "Viewport_Camera_Parent" &&
            node.name !== "viewport_camera" &&
            !node.name.includes("gizmo") &&
            !node.name.includes("Gizmo")
    );

    if (countBadge) {
        countBadge.textContent = `${visibleNodes.length}`;
    }

    hierarchyTreeEl.innerHTML = "";

    if (visibleNodes.length === 0) {
        const emptyLi = document.createElement("li");
        emptyLi.className = "hierarchy-empty";
        emptyLi.textContent = "No objects in scene";
        hierarchyTreeEl.appendChild(emptyLi);
        return;
    }

    // Build parent-child tree mapping
    const rootNodes: Node[] = [];
    const childrenMap = new Map<number, Node[]>();

    visibleNodes.forEach(node => {
        const parent = node.parent;
        if (parent && visibleNodes.some(n => n.uniqueId === parent.uniqueId)) {
            if (!childrenMap.has(parent.uniqueId)) {
                childrenMap.set(parent.uniqueId, []);
            }
            childrenMap.get(parent.uniqueId)!.push(node);
        } else {
            rootNodes.push(node);
        }
    });

    const renderNode = (node: Node, depth: number) => {
        const li = document.createElement("li");
        li.className = `hierarchy-item ${node.uniqueId === activeSelectedUniqueId ? "selected" : ""}`;
        li.style.paddingLeft = `${16 + depth * 14}px`;

        const iconSpan = document.createElement("span");
        iconSpan.className = "node-type-icon";
        iconSpan.textContent = getNodeIcon(node);

        const nameSpan = document.createElement("span");
        nameSpan.className = "node-name";
        nameSpan.textContent = node.name || `Node_${node.uniqueId}`;

        li.appendChild(iconSpan);
        li.appendChild(nameSpan);

        if (node instanceof AbstractMesh) {
            const eyeBtn = document.createElement("button");
            eyeBtn.className = "node-visibility-btn";
            eyeBtn.title = "Toggle Visibility";
            eyeBtn.innerHTML = node.isVisible ? "👁️" : "🙈";

            eyeBtn.addEventListener("click", (e) => {
                e.stopPropagation();
                node.isVisible = !node.isVisible;
                eyeBtn.innerHTML = node.isVisible ? "👁️" : "🙈";
                EngineRegistry.RequestRender?.(10);
            });

            li.appendChild(eyeBtn);
        }

        li.addEventListener("click", () => {
            activeSelectedUniqueId = node.uniqueId;
            EngineRegistry.SelectNode?.(node);
            updateHierarchyUI();
            EngineRegistry.RequestRender?.(10);
        });

        hierarchyTreeEl.appendChild(li);

        const children = childrenMap.get(node.uniqueId);
        if (children) {
            children.forEach(child => renderNode(child, depth + 1));
        }
    };

    rootNodes.forEach(root => renderNode(root, 0));
}

function getNodeIcon(node: Node): string {
    if (node instanceof Camera) return "📷";
    if (node instanceof Light) return "💡";
    if (node instanceof AbstractMesh) {
        if (node.name.toLowerCase().includes("cube") || node.name.toLowerCase().includes("box")) return "📦";
        if (node.name.toLowerCase().includes("ground") || node.name.toLowerCase().includes("plane")) return "🏁";
        if (node.name.toLowerCase().includes("sphere")) return "🌐";
        return "🔷";
    }
    if (node instanceof TransformNode) return "📍";
    return "🔹";
}
