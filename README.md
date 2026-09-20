# Eunoia-Editor

A 3D Game Engine and real-time Level Editor built with **DirectX 12 (D3D12)**, **C++17**, **Dear ImGui** (docked Unreal Engine 5 layout), and **ImGuizmo**.

## Features

- **DirectX 12 Low-Level Graphics Backend**:
  - `ID3D12Device`, Direct `ID3D12CommandQueue`, and `ID3D12GraphicsCommandList`.
  - Double-buffered `IDXGISwapChain3` presentation (`DXGI_SWAP_EFFECT_FLIP_DISCARD`).
  - Descriptor Heaps for RTV, DSV, and Shader-Visible SRV (ImGui & textures).
  - Synchronized GPU fences and CPU pacing via Win32 events.
  - Runtime HLSL compilation via `D3DCompile` (`vs_5_0` and `ps_5_0`).
- **Docked Three-Panel Editor Layout**:
  - **Outliner (Left Sidebar)**: Actor hierarchy, filter search, visibility toggles, duplicate & delete.
  - **Details (Right Sidebar)**: Transform Location, Rotation, Scale with colored axis pills (X=Red, Y=Green, Z=Blue), Mobility (`Static`, `Stationary`, `Movable`), materials, and auto-rotation settings.
  - **Content Browser (Bottom Dock)**: Fast 3D primitive shape cards (`SM_Cube`, `SM_Plane`, `SM_Sphere`, `SM_Cylinder`, `SM_Pyramid`, `SM_Torus`), Output Log console, and World Settings.
  - **Central 3D Level Viewport**: Floating camera and view overlays (Perspective/Top/Front/Side, Lit/Unlit, Cam speed, FPS).
- **Real-Time Procedural Mesh Generation**:
  - Cube, Plane, Sphere, Cylinder, Pyramid, Torus, and Ground Grid.
  - Dynamic GPU upload buffers for real-time vertex/index updates.
- **3D Transform Gizmos (ImGuizmo)**:
  - Translate (W), Rotate (E), and Scale (R) modes.
  - World vs. Local coordinate spaces (Q) with grid snapping support.
- **Unreal Engine Viewport Fly Camera & Navigation**:
  - **Hold RMB + W/S/A/D/E/Q**: 6-DOF free flight (Forward, Backward, Strafe Left/Right, Move Up/Down).
  - **Hold RMB + Mouse Movement**: 360-degree free look around.
  - **Hold RMB + Scroll Wheel**: Adjust fly camera movement speed dynamically.
  - **Hold Shift while flying**: Sprint boost (2.5x speed).
  - **F Key**: Focus and frame selected actor.
  - **Alt + LMB + Drag**: Orbit around pivot/selected object.
  - **Alt + RMB + Drag**: Smooth dolly/zoom.
  - **Alt + MMB + Drag**: Pan/track viewport.
  - **F11 Key**: Toggle immersive fullscreen viewport mode.
  - **G Key**: Toggle Game View (hides ground grid & transform gizmos).

## Building & Running

### Requirements
- Windows 10/11 (64-bit)
- DirectX 12 capable GPU
- C++17 compiler (GCC / MinGW / Clang / MSVC)

### Quick Start
To build and launch the editor:
```cmd
run.bat
```

Or build manually:
```cmd
build.bat
```
This produces `Eunoia-Editor.exe`.