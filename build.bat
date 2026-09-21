@echo off
setlocal enabledelayedexpansion
set "PROJECT_DIR=%~dp0"
if "%PROJECT_DIR:~-1%"=="\" set "PROJECT_DIR=%PROJECT_DIR:~0,-1%"

set "COMPILER=%PROJECT_DIR%\tools\w64devkit\bin\g++.exe"
if not exist "%COMPILER%" (
    where g++ >nul 2>&1
    if !ERRORLEVEL! equ 0 (
        set "COMPILER=g++"
    ) else (
        echo [ERROR] C++ compiler not found!
        exit /b 1
    )
)

echo [INFO] Compiling Eunoia-Editor (DirectX 12) with Dear ImGui...
"%COMPILER%" -std=c++17 -O2 ^
    "-I%PROJECT_DIR%\include" ^
    "-I%PROJECT_DIR%\EngineCore\include" ^
    "-I%PROJECT_DIR%\EnginePlatform\include" ^
    "-I%PROJECT_DIR%\EngineRHI\include" ^
    "-I%PROJECT_DIR%\EngineRenderer\include" ^
    "-I%PROJECT_DIR%\EngineScene\include" ^
    "-I%PROJECT_DIR%\EngineAssets\include" ^
    "-I%PROJECT_DIR%\Editor\include" ^
    "-I%PROJECT_DIR%\deps\tinyobj" ^
    "-I%PROJECT_DIR%\deps\cgltf" ^
    "-I%PROJECT_DIR%\deps\ufbx" ^
    "-I%PROJECT_DIR%\deps\imgui" ^
    "-I%PROJECT_DIR%\deps\imgui\backends" ^
    "-I%PROJECT_DIR%\deps\glfw-3.5.1.bin.WIN64\include" ^
    "-I%PROJECT_DIR%\deps\glm" ^
    "-I%PROJECT_DIR%" ^
    "%PROJECT_DIR%\deps\ufbx\ufbx.c" ^
    "%PROJECT_DIR%\src\MeshImporter.cpp" ^
    "%PROJECT_DIR%\Cube.cpp" ^
    "%PROJECT_DIR%\src\EngineUI.cpp" ^
    "%PROJECT_DIR%\src\EunoiaBehaviour.cpp" ^
    "%PROJECT_DIR%\src\TextureManager.cpp" ^
    "%PROJECT_DIR%\src\AssetRegistry.cpp" ^
    "%PROJECT_DIR%\src\AssetManager.cpp" ^
    "%PROJECT_DIR%\projects\test\Behaviours\Behaviours\FPS_Player.cpp" ^
    "%PROJECT_DIR%\deps\imgui\imgui.cpp" ^
    "%PROJECT_DIR%\deps\imgui\imgui_draw.cpp" ^
    "%PROJECT_DIR%\deps\imgui\imgui_tables.cpp" ^
    "%PROJECT_DIR%\deps\imgui\imgui_widgets.cpp" ^
    "%PROJECT_DIR%\deps\imgui\backends\imgui_impl_glfw.cpp" ^
    "%PROJECT_DIR%\deps\imgui\backends\imgui_impl_dx12.cpp" ^
    "%PROJECT_DIR%\deps\imgui\ImGuizmo.cpp" ^
    "-L%PROJECT_DIR%\deps\glfw-3.5.1.bin.WIN64\lib-mingw-w64" ^
    -lglfw3 -ld3d12 -ldxgi -ld3dcompiler -lgdi32 -limm32 -lcomdlg32 ^
    -o "%PROJECT_DIR%\Eunoia-Editor.exe"

if %ERRORLEVEL% equ 0 (
    echo [SUCCESS] Eunoia-Editor.exe built successfully!
) else (
    echo [ERROR] Build failed.
    exit /b %ERRORLEVEL%
)
