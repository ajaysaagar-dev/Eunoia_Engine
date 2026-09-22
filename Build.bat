@echo off
setlocal enabledelayedexpansion
set "PROJECT_DIR=%~dp0"
if "%PROJECT_DIR:~-1%"=="\" set "PROJECT_DIR=%PROJECT_DIR:~0,-1%"

:: Build directories: Build\Engine for engine, Build subfolders as needed, and Projects\<Game>\Build for games
set "BUILD_ROOT=%PROJECT_DIR%\Build"
set "OUTPUT_DIR=%BUILD_ROOT%\Engine"
set "INTERMEDIATES_DIR=%BUILD_ROOT%\Intermediates"
set "TESTS_BUILD_DIR=%BUILD_ROOT%\Tests"
set "RUNTIME_BUILD_DIR=%BUILD_ROOT%\Runtime"
set "GAME_BUILD_DIR=%PROJECT_DIR%\Projects\test\Build"

if not exist "%BUILD_ROOT%" mkdir "%BUILD_ROOT%"
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
if not exist "%INTERMEDIATES_DIR%" mkdir "%INTERMEDIATES_DIR%"
if not exist "%TESTS_BUILD_DIR%" mkdir "%TESTS_BUILD_DIR%"
if not exist "%RUNTIME_BUILD_DIR%" mkdir "%RUNTIME_BUILD_DIR%"
if not exist "%GAME_BUILD_DIR%" mkdir "%GAME_BUILD_DIR%"

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

echo [INFO] Compiling Eunoia-Editor (DirectX 12) with Dear ImGui into %OUTPUT_DIR%...
"%COMPILER%" -std=c++17 -O2 ^
    "-I%PROJECT_DIR%\Engine\EngineCore\Include" ^
    "-I%PROJECT_DIR%\Engine\EnginePlatform\Include" ^
    "-I%PROJECT_DIR%\Engine\EngineRHI\Include" ^
    "-I%PROJECT_DIR%\Engine\EngineRenderer\Include" ^
    "-I%PROJECT_DIR%\Engine\EngineScene\Include" ^
    "-I%PROJECT_DIR%\Engine\EngineAssets\Include" ^
    "-I%PROJECT_DIR%\Engine\EunoiaPluginCore\Include" ^
    "-I%PROJECT_DIR%\Editor\Editor\Include" ^
    "-I%PROJECT_DIR%\deps\tinyobj" ^
    "-I%PROJECT_DIR%\deps\cgltf" ^
    "-I%PROJECT_DIR%\deps\ufbx" ^
    "-I%PROJECT_DIR%\deps\imgui" ^
    "-I%PROJECT_DIR%\deps\imgui\backends" ^
    "-I%PROJECT_DIR%\deps\glfw-3.5.1.bin.WIN64\include" ^
    "-I%PROJECT_DIR%\deps\glm" ^
    "-I%PROJECT_DIR%\deps\json" ^
    "-I%PROJECT_DIR%" ^
    "%PROJECT_DIR%\deps\ufbx\ufbx.c" ^
    "%PROJECT_DIR%\Engine\EngineAssets\Src\MeshImporter.cpp" ^
    "%PROJECT_DIR%\Engine\EngineAssets\Src\TextureManager.cpp" ^
    "%PROJECT_DIR%\Engine\EngineAssets\Src\AssetRegistry.cpp" ^
    "%PROJECT_DIR%\Engine\EngineAssets\Src\AssetManager.cpp" ^
    "%PROJECT_DIR%\Engine\EngineScene\Src\EunoiaBehaviour.cpp" ^
    "%PROJECT_DIR%\Editor\Editor\Src\Main.cpp" ^
    "%PROJECT_DIR%\Editor\Editor\Src\EngineUI.cpp" ^
    "%PROJECT_DIR%\Projects\test\Behaviours\Behaviours\FPS_Player.cpp" ^
    "%PROJECT_DIR%\deps\imgui\imgui.cpp" ^
    "%PROJECT_DIR%\deps\imgui\imgui_draw.cpp" ^
    "%PROJECT_DIR%\deps\imgui\imgui_tables.cpp" ^
    "%PROJECT_DIR%\deps\imgui\imgui_widgets.cpp" ^
    "%PROJECT_DIR%\deps\imgui\backends\imgui_impl_glfw.cpp" ^
    "%PROJECT_DIR%\deps\imgui\backends\imgui_impl_dx12.cpp" ^
    "%PROJECT_DIR%\deps\imgui\ImGuizmo.cpp" ^
    "-L%PROJECT_DIR%\deps\glfw-3.5.1.bin.WIN64\lib-mingw-w64" ^
    -lglfw3 -ld3d12 -ldxgi -ld3dcompiler -lgdi32 -limm32 -lcomdlg32 ^
    -o "%OUTPUT_DIR%\Eunoia-Editor.exe"

if !ERRORLEVEL! neq 0 (
    echo [ERROR] Engine build failed.
    exit /b !ERRORLEVEL!
)

echo [SUCCESS] Eunoia-Editor.exe built successfully in %OUTPUT_DIR%.

echo [INFO] Packaging binaries, code, headers, and assets into %OUTPUT_DIR%...

:: Copy external DLLs if present
if exist "%PROJECT_DIR%\deps\glfw-3.5.1.bin.WIN64\lib-mingw-w64\glfw3.dll" (
    copy /y "%PROJECT_DIR%\deps\glfw-3.5.1.bin.WIN64\lib-mingw-w64\glfw3.dll" "%OUTPUT_DIR%\" >nul 2>&1
)
if exist "%PROJECT_DIR%\Plugins\primitives\primitives.dll" (
    copy /y "%PROJECT_DIR%\Plugins\primitives\primitives.dll" "%OUTPUT_DIR%\" >nul 2>&1
)

:: Copy resources and shaders
xcopy /s /e /y /d /q /i "%PROJECT_DIR%\Resources" "%OUTPUT_DIR%\Resources\" >nul 2>&1
xcopy /s /e /y /d /q /i "%PROJECT_DIR%\Shaders" "%OUTPUT_DIR%\Shaders\" >nul 2>&1

:: Copy engine modular architecture, editor, and runtime
xcopy /s /e /y /d /q /i "%PROJECT_DIR%\Engine" "%OUTPUT_DIR%\Engine\" >nul 2>&1
xcopy /s /e /y /d /q /i "%PROJECT_DIR%\Editor" "%OUTPUT_DIR%\Editor\" >nul 2>&1
xcopy /s /e /y /d /q /i "%PROJECT_DIR%\Runtime" "%OUTPUT_DIR%\Runtime\" >nul 2>&1
xcopy /s /e /y /d /q /i "%PROJECT_DIR%\Docs" "%OUTPUT_DIR%\Docs\" >nul 2>&1

:: Copy plugins and sample projects
xcopy /s /e /y /d /q /i "%PROJECT_DIR%\Plugins" "%OUTPUT_DIR%\Plugins\" >nul 2>&1
xcopy /s /e /y /d /q /i "%PROJECT_DIR%\Projects" "%OUTPUT_DIR%\Projects\" >nul 2>&1

:: Copy dependencies needed for project compiling (e.g. GLM)
xcopy /s /e /y /d /q /i "%PROJECT_DIR%\deps\glm" "%OUTPUT_DIR%\Deps\glm\" >nul 2>&1

:: Generate run script inside Build\Engine
(
echo @echo off
echo cd /d "%%~dp0"
echo echo [INFO] Starting Eunoia-Editor [DirectX 12]...
echo start "" "Eunoia-Editor.exe"
) > "%OUTPUT_DIR%\Run.bat"

echo [SUCCESS] Complete game engine distribution packaged into: %OUTPUT_DIR%
echo [INFO] Contents of %OUTPUT_DIR%:
dir /b "%OUTPUT_DIR%"

:: ─────────────────────────────────────────────────────────────────────────────
:: Build Standalone Game Runtime into the respective game project's Build folder
:: ─────────────────────────────────────────────────────────────────────────────
echo.
echo [INFO] Building Game Project into %GAME_BUILD_DIR%...
"%COMPILER%" -std=c++17 -O2 ^
    "-I%PROJECT_DIR%\Engine\EngineCore\Include" ^
    "-I%PROJECT_DIR%\Engine\EnginePlatform\Include" ^
    "-I%PROJECT_DIR%\deps\glfw-3.5.1.bin.WIN64\include" ^
    "%PROJECT_DIR%\Runtime\Src\Main.cpp" ^
    "-L%PROJECT_DIR%\deps\glfw-3.5.1.bin.WIN64\lib-mingw-w64" ^
    -lglfw3 -lgdi32 ^
    -o "%GAME_BUILD_DIR%\test.exe"

if !ERRORLEVEL! equ 0 (
    :: Copy external runtime DLLs
    if exist "%PROJECT_DIR%\deps\glfw-3.5.1.bin.WIN64\lib-mingw-w64\glfw3.dll" (
        copy /y "%PROJECT_DIR%\deps\glfw-3.5.1.bin.WIN64\lib-mingw-w64\glfw3.dll" "%GAME_BUILD_DIR%\" >nul 2>&1
    )
    if exist "%PROJECT_DIR%\Plugins\primitives\primitives.dll" (
        copy /y "%PROJECT_DIR%\Plugins\primitives\primitives.dll" "%GAME_BUILD_DIR%\" >nul 2>&1
    )

    :: Copy engine shaders and resources needed by standalone game
    xcopy /s /e /y /d /q /i "%PROJECT_DIR%\Shaders" "%GAME_BUILD_DIR%\Shaders\" >nul 2>&1
    xcopy /s /e /y /d /q /i "%PROJECT_DIR%\Resources" "%GAME_BUILD_DIR%\Resources\" >nul 2>&1

    :: Copy project cooked content, scenes, levels, registry, materials, models, and behaviours
    if exist "%PROJECT_DIR%\Projects\test\Cooked" xcopy /s /e /y /d /q /i "%PROJECT_DIR%\Projects\test\Cooked" "%GAME_BUILD_DIR%\Cooked\" >nul 2>&1
    if exist "%PROJECT_DIR%\Projects\test\Scenes" xcopy /s /e /y /d /q /i "%PROJECT_DIR%\Projects\test\Scenes" "%GAME_BUILD_DIR%\Scenes\" >nul 2>&1
    if exist "%PROJECT_DIR%\Projects\test\Levels" xcopy /s /e /y /d /q /i "%PROJECT_DIR%\Projects\test\Levels" "%GAME_BUILD_DIR%\Levels\" >nul 2>&1
    if exist "%PROJECT_DIR%\Projects\test\Registry" xcopy /s /e /y /d /q /i "%PROJECT_DIR%\Projects\test\Registry" "%GAME_BUILD_DIR%\Registry\" >nul 2>&1
    if exist "%PROJECT_DIR%\Projects\test\Materials" xcopy /s /e /y /d /q /i "%PROJECT_DIR%\Projects\test\Materials" "%GAME_BUILD_DIR%\Materials\" >nul 2>&1
    if exist "%PROJECT_DIR%\Projects\test\Models" xcopy /s /e /y /d /q /i "%PROJECT_DIR%\Projects\test\Models" "%GAME_BUILD_DIR%\Models\" >nul 2>&1
    if exist "%PROJECT_DIR%\Projects\test\Behaviours" xcopy /s /e /y /d /q /i "%PROJECT_DIR%\Projects\test\Behaviours" "%GAME_BUILD_DIR%\Behaviours\" >nul 2>&1

    :: Copy project root files (*.emat, *.assetmeta, *.json, *.cpp)
    copy /y "%PROJECT_DIR%\Projects\test\*.emat" "%GAME_BUILD_DIR%\" >nul 2>&1
    copy /y "%PROJECT_DIR%\Projects\test\*.assetmeta" "%GAME_BUILD_DIR%\" >nul 2>&1
    copy /y "%PROJECT_DIR%\Projects\test\*.json" "%GAME_BUILD_DIR%\" >nul 2>&1
    copy /y "%PROJECT_DIR%\Projects\test\*.cpp" "%GAME_BUILD_DIR%\" >nul 2>&1

    (
    echo @echo off
    echo cd /d "%%~dp0"
    echo echo [INFO] Starting test game...
    echo start "" "test.exe"
    ) > "%GAME_BUILD_DIR%\Run.bat"
    echo [SUCCESS] Game built and packaged successfully into %GAME_BUILD_DIR%.
    echo [INFO] Contents of %GAME_BUILD_DIR%:
    dir /b "%GAME_BUILD_DIR%"
) else (
    echo [WARNING] Game build encountered an issue.
)
