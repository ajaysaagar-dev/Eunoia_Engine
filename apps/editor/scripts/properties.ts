import { AbstractMesh, Light, Node, TransformNode, Vector3, Quaternion } from "@babylonjs/core";
import EngineRegistry from "../../../engine/registry.plugins";
import { updateHierarchyUI } from "./hierarchy";

let activeNode: Node | null = null;
let updateInterval: any = null;

export function initPropertiesUI() {
    const origOnSelectedChanged = EngineRegistry.OnSelectedNodeChanged;
    EngineRegistry.OnSelectedNodeChanged = (node: any) => {
        if (origOnSelectedChanged) origOnSelectedChanged(node);
        inspectNode(node);
    };

    // Check initially selected node
    const current = EngineRegistry.GetSelectedNode?.();
    inspectNode(current || null);
}

export function inspectNode(node: Node | null) {
    activeNode = node;
    const badgeEl = document.getElementById("prop-object-name");
    const contentEl = document.getElementById("properties-content");

    if (!contentEl) return;

    if (!activeNode) {
        if (badgeEl) badgeEl.textContent = "No Selection";
        contentEl.innerHTML = `<div class="properties-empty">Select an object to inspect properties</div>`;
        if (updateInterval) {
            clearInterval(updateInterval);
            updateInterval = null;
        }
        return;
    }

    if (badgeEl) {
        badgeEl.textContent = activeNode.name || `Node_${activeNode.uniqueId}`;
        badgeEl.title = activeNode.name;
    }

    renderPropertiesForm(activeNode);
}

function renderPropertiesForm(node: Node) {
    const contentEl = document.getElementById("properties-content");
    if (!contentEl) return;

    contentEl.innerHTML = "";

    // 1. General Node Group
    const generalGroup = document.createElement("div");
    generalGroup.className = "prop-group";

    const generalTitle = document.createElement("div");
    generalTitle.className = "prop-group-title";
    generalTitle.textContent = "General";
    generalGroup.appendChild(generalTitle);

    // Name row
    const nameRow = document.createElement("div");
    nameRow.className = "prop-row";
    nameRow.innerHTML = `<span class="prop-label">Name</span>`;
    const nameInput = document.createElement("input");
    nameInput.type = "text";
    nameInput.className = "prop-text-input";
    nameInput.value = node.name;
    nameInput.addEventListener("input", () => {
        node.name = nameInput.value;
        const badgeEl = document.getElementById("prop-object-name");
        if (badgeEl) badgeEl.textContent = node.name;
        updateHierarchyUI();
    });
    nameRow.appendChild(nameInput);
    generalGroup.appendChild(nameRow);

    contentEl.appendChild(generalGroup);

    // 2. Transform Group (if TransformNode or Mesh)
    if (node instanceof TransformNode || node instanceof AbstractMesh) {
        const transformGroup = document.createElement("div");
        transformGroup.className = "prop-group";

        const transformTitle = document.createElement("div");
        transformTitle.className = "prop-group-title";
        transformTitle.textContent = "Transform";
        transformGroup.appendChild(transformTitle);

        // Position
        const posRow = createVector3Row("Position", node.position, (newPos) => {
            node.position.copyFrom(newPos);
            EngineRegistry.RequestRender?.(10);
        });
        transformGroup.appendChild(posRow);

        // Rotation (Degrees)
        const rotRad = node.rotationQuaternion
            ? node.rotationQuaternion.toEulerAngles()
            : node.rotation;
        const rotDeg = new Vector3(
            rotRad.x * (180 / Math.PI),
            rotRad.y * (180 / Math.PI),
            rotRad.z * (180 / Math.PI)
        );

        const rotRow = createVector3Row("Rotation", rotDeg, (newRotDeg) => {
            const newRotRad = new Vector3(
                newRotDeg.x * (Math.PI / 180),
                newRotDeg.y * (Math.PI / 180),
                newRotDeg.z * (Math.PI / 180)
            );
            if (node.rotationQuaternion) {
                node.rotationQuaternion = Quaternion.FromEulerVector(newRotRad);
            } else {
                node.rotation.copyFrom(newRotRad);
            }
            EngineRegistry.RequestRender?.(10);
        });
        transformGroup.appendChild(rotRow);

        // Scale
        const scaleRow = createVector3Row("Scale", node.scaling, (newScale) => {
            node.scaling.copyFrom(newScale);
            EngineRegistry.RequestRender?.(10);
        });
        transformGroup.appendChild(scaleRow);

        contentEl.appendChild(transformGroup);
    }

    // 3. Light Properties (if Light)
    if (node instanceof Light) {
        const lightGroup = document.createElement("div");
        lightGroup.className = "prop-group";

        const lightTitle = document.createElement("div");
        lightTitle.className = "prop-group-title";
        lightTitle.textContent = "Light Component";
        lightGroup.appendChild(lightTitle);

        const intensityRow = document.createElement("div");
        intensityRow.className = "prop-row";
        intensityRow.innerHTML = `<span class="prop-label">Intensity</span>`;
        const intensityInput = document.createElement("input");
        intensityInput.type = "number";
        intensityInput.step = "0.5";
        intensityInput.className = "prop-number-input";
        intensityInput.value = node.intensity.toString();
        intensityInput.addEventListener("input", () => {
            const val = parseFloat(intensityInput.value);
            if (!isNaN(val)) {
                node.intensity = val;
                EngineRegistry.RequestRender?.(10);
            }
        });
        intensityRow.appendChild(intensityInput);
        lightGroup.appendChild(intensityRow);

        contentEl.appendChild(lightGroup);
    }

    // Start live refresh interval to sync numbers during viewport gizmo drag
    if (updateInterval) clearInterval(updateInterval);
    updateInterval = setInterval(() => {
        if (!activeNode) return;
        const activeEl = document.activeElement;
        if (activeEl && (activeEl.tagName === "INPUT" || activeEl.tagName === "TEXTAREA")) {
            return; // Don't overwrite if user is actively typing
        }
        syncInputsWithNode(activeNode);
    }, 150);
}

function createVector3Row(label: string, initialVec: Vector3, onChange: (vec: Vector3) => void): HTMLElement {
    const row = document.createElement("div");
    row.className = "prop-row";

    const labelSpan = document.createElement("span");
    labelSpan.className = "prop-label";
    labelSpan.textContent = label;
    row.appendChild(labelSpan);

    const inputGroup = document.createElement("div");
    inputGroup.className = "prop-input-group";

    const currentVec = initialVec.clone();

    ["x", "y", "z"].forEach((axis) => {
        const input = document.createElement("input");
        input.type = "number";
        input.step = "0.1";
        input.dataset.axis = axis;
        input.dataset.label = label;
        input.className = "prop-number-input";
        input.value = currentVec[axis as "x" | "y" | "z"].toFixed(2);

        input.addEventListener("input", () => {
            const val = parseFloat(input.value);
            if (!isNaN(val)) {
                currentVec[axis as "x" | "y" | "z"] = val;
                onChange(currentVec);
            }
        });

        inputGroup.appendChild(input);
    });

    row.appendChild(inputGroup);
    return row;
}

function syncInputsWithNode(node: Node) {
    if (!(node instanceof TransformNode || node instanceof AbstractMesh)) return;

    const contentEl = document.getElementById("properties-content");
    if (!contentEl) return;

    const inputs = contentEl.querySelectorAll<HTMLInputElement>(".prop-number-input");
    inputs.forEach((input) => {
        const axis = input.dataset.axis as "x" | "y" | "z";
        const label = input.dataset.label;
        if (!axis || !label) return;

        if (label === "Position") {
            input.value = node.position[axis].toFixed(2);
        } else if (label === "Rotation") {
            const rotRad = node.rotationQuaternion
                ? node.rotationQuaternion.toEulerAngles()
                : node.rotation;
            const deg = rotRad[axis] * (180 / Math.PI);
            input.value = deg.toFixed(2);
        } else if (label === "Scale") {
            input.value = node.scaling[axis].toFixed(2);
        }
    });
}
