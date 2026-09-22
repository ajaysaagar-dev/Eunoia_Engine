@echo off
setlocal enabledelayedexpansion
set "PROJECT_DIR=%~dp0"
if "%PROJECT_DIR:~-1%"=="\" set "PROJECT_DIR=%PROJECT_DIR:~0,-1%"

set "OUTPUT_DIR=%PROJECT_DIR%\EngineBuild"
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

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
    -o "%OUTPUT_DIR%\Eunoia-Editor.exe"

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed.
    exit /b %ERRORLEVEL%
)

echo [SUCCESS] Eunoia-Editor.exe built successfully in %OUTPUT_DIR%!

:: Sync executable to root for backwards-compatible launching
copy /y "%OUTPUT_DIR%\Eunoia-Editor.exe" "%PROJECT_DIR%\Eunoia-Editor.exe" >nul 2>&1

echo [INFO] Packaging binaries, code, headers, and assets into .\EngineBuild...

:: Copy external DLLs if present
if exist "%PROJECT_DIR%\deps\glfw-3.5.1.bin.WIN64\lib-mingw-w64\glfw3.dll" (
    copy /y "%PROJECT_DIR%\deps\glfw-3.5.1.bin.WIN64\lib-mingw-w64\glfw3.dll" "%OUTPUT_DIR%\" >nul 2>&1
)
if exist "%PROJECT_DIR%\builds\*.dll" (
    copy /y "%PROJECT_DIR%\builds\*.dll" "%OUTPUT_DIR%\" >nul 2>&1
)

:: Copy resources and shaders
xcopy /s /e /y /d /q /i "%PROJECT_DIR%\resources" "%OUTPUT_DIR%\resources\" >nul 2>&1
xcopy /s /e /y /d /q /i "%PROJECT_DIR%\shaders" "%OUTPUT_DIR%\shaders\" >nul 2>&1

:: Copy engine headers and code
xcopy /s /e /y /d /q /i "%PROJECT_DIR%\include" "%OUTPUT_DIR%\include\" >nul 2>&1
xcopy /s /e /y /d /q /i "%PROJECT_DIR%\src" "%OUTPUT_DIR%\src\" >nul 2>&1
xcopy /s /e /y /d /q /i "%PROJECT_DIR%\EngineCore" "%OUTPUT_DIR%\EngineCore\" >nul 2>&1
xcopy /s /e /y /d /q /i "%PROJECT_DIR%\EnginePlatform" "%OUTPUT_DIR%\EnginePlatform\" >nul 2>&1
xcopy /s /e /y /d /q /i "%PROJECT_DIR%\EngineRHI" "%OUTPUT_DIR%\EngineRHI\" >nul 2>&1
xcopy /s /e /y /d /q /i "%PROJECT_DIR%\EngineRenderer" "%OUTPUT_DIR%\EngineRenderer\" >nul 2>&1
xcopy /s /e /y /d /q /i "%PROJECT_DIR%\EngineScene" "%OUTPUT_DIR%\EngineScene\" >nul 2>&1
xcopy /s /e /y /d /q /i "%PROJECT_DIR%\EngineAssets" "%OUTPUT_DIR%\EngineAssets\" >nul 2>&1
xcopy /s /e /y /d /q /i "%PROJECT_DIR%\Editor" "%OUTPUT_DIR%\Editor\" >nul 2>&1
xcopy /s /e /y /d /q /i "%PROJECT_DIR%\EunoiaPluginCore" "%OUTPUT_DIR%\EunoiaPluginCore\" >nul 2>&1

:: Copy plugins and sample projects
xcopy /s /e /y /d /q /i "%PROJECT_DIR%\plugins" "%OUTPUT_DIR%\plugins\" >nul 2>&1
xcopy /s /e /y /d /q /i "%PROJECT_DIR%\projects" "%OUTPUT_DIR%\projects\" >nul 2>&1

:: Copy dependencies needed for project compiling (e.g. GLM)
xcopy /s /e /y /d /q /i "%PROJECT_DIR%\deps\glm" "%OUTPUT_DIR%\deps\glm\" >nul 2>&1

:: Generate run script inside EngineBuild
(
echo @echo off
echo cd /d "%%~dp0"
echo echo [INFO] Starting Eunoia-Editor (DirectX 12^)...
echo start "" "Eunoia-Editor.exe"
) > "%OUTPUT_DIR%\run.bat"

echo [SUCCESS] Complete game engine distribution packaged into: %OUTPUT_DIR%
echo [INFO] Contents of %OUTPUT_DIR%:
dir /b "%OUTPUT_DIR%"
