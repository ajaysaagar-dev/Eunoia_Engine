<div align="center">

# 🎮 Eunoia Engine

**A Modular, Plugin-Driven 3D & 2D Game Engine and Editor built with TypeScript, Babylon.js, and Electron.**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Engine: TypeScript](https://img.shields.io/badge/Engine-TypeScript-3178C6.svg?logo=typescript&logoColor=white)](https://www.typescriptlang.org/)
[![Renderer: Babylon.js](https://img.shields.io/badge/Renderer-Babylon.js-BB464B.svg)](https://www.babylonjs.com/)
[![Runtime: Electron](https://img.shields.io/badge/Runtime-Electron-47848F.svg?logo=electron&logoColor=white)](https://www.electronjs.org/)

</div>

---

## 🌟 Overview

**Eunoia Engine** is a high-performance, modular game engine designed for rapid 3D/2D game development and interactive visualization. Built around a decoupled, plugin-based architecture, Eunoia Engine allows developers to extend engine subsystems, customize the rendering pipeline, and craft custom tooling through isolated plugins.

---

## 🏛️ Architecture & Plugin Ecosystem

Eunoia Engine isolates core game engine features into specialized plugin modules:

```text
Eunoia Engine Core
├── 🎥 Camera Plugin      - Perspective, ArcRotate, FreeCam, and Cinematic Controllers
├── 🎨 Renderer Plugin    - Babylon.js WebGL/WebGPU Rendering & Canvas Orchestration
├── 🌐 Scene Graph        - Entity Hierarchy, Spatial Transformations, and Lifecycle
├── 💡 Lights & Shadows   - Directional, Point, Spot, Hemispheric Lights & Cascade Shadows
├── 🧱 Meshes & Geometry  - Parametric Meshes, glTF/GLB Asset Loaders, and Geometry Buffers
├── 🔮 Materials & PBR    - Physically Based Rendering (PBR), Shaders, and Texture Maps
├── 📐 Gizmos & Tools     - Real-Time 3D Translation, Rotation, and Scale Gizmos
├── 📁 File System        - Local Asset Pipeline and Project Serialization
├── 📊 Performance Stats  - Real-Time FPS, Frame Time, Draw Calls, and Memory Telemetry
└── 🖥️ Editor Interface   - Desktop IDE Window Powered by Electron and Vite
```

---

## 🚀 Getting Started

### Prerequisites

- **Node.js**: >= 18.0.0
- **pnpm** or **npm**

### Installation

```bash
# 1. Clone the repository
git clone https://github.com/ajaysaagar-dev/Eunoia_Engine.git
cd Eunoia_Engine

# 2. Install dependencies
npm install
```

### Launching the Engine Editor

```bash
npm run engine
```

---

## 🛠️ Project Structure

```text
Eunoia_Engine/
├── apps/
│   └── editor/          # Integrated Game Engine Editor application
├── engine/              # Engine plugin registry and state management
├── plugins/             # Discrete engine subsystems (camera, lights, meshes, renderer)
├── types/               # TypeScript definitions and interface declarations
├── window/              # Electron main process entry point and window manager
└── vite.config.ts       # Bundler and developer environment configuration
```

---

## 🧪 Development & Contributions

We welcome contributions from the community! Please read our [CONTRIBUTING.md](CONTRIBUTING.md) to learn about our development process and coding guidelines.

---

## 🛡️ Security

For vulnerability disclosure and security policies, refer to [SECURITY.md](SECURITY.md).

---

## 📄 License

This project is licensed under the [GNU General Public License v3.0](LICENSE).
