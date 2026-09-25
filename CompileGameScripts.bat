@echo off
setlocal enabledelayedexpansion

:: ============================================================================
:: CompileGameScripts.bat
:: Compiles user behaviour scripts and packages standalone game executable.
:: Usage: CompileGameScripts.bat [ProjectPath]
:: ============================================================================

set "ENGINE_ROOT=%~dp0"
if "%ENGINE_ROOT:~-1%"=="\" set "ENGINE_ROOT=%ENGINE_ROOT:~0,-1%"

set "PROJ_PATH=%~1"
if "%PROJ_PATH%"=="" (
    set "PROJ_PATH=%ENGINE_ROOT%\..\Projects\Unknown-13-Test"
)

:: Canonicalize project path
for %%i in ("%PROJ_PATH%") do set "PROJ_PATH=%%~fi"
for %%i in ("%PROJ_PATH%") do set "PROJ_NAME=%%~nxi"

set "PROJ_BUILD=%PROJ_PATH%\Build"
set "PROJ_CONTENT=%PROJ_PATH%\Content"

echo ============================================================================
echo [CompileGameScripts] Project: %PROJ_NAME%
echo [CompileGameScripts] Path:    %PROJ_PATH%
echo [CompileGameScripts] Output:  %PROJ_BUILD%
echo ============================================================================

:: Locate compiler
set "COMPILER="
if exist "%ENGINE_ROOT%\tools\w64devkit\bin\g++.exe" (
    set "COMPILER=%ENGINE_ROOT%\tools\w64devkit\bin\g++.exe"
) else (
    where g++.exe >nul 2>&1
    if !ERRORLEVEL! equ 0 (
        set "COMPILER=g++.exe"
    )
)

if not defined COMPILER (
    echo [ERROR] No C++ compiler found. Please install w64devkit in %ENGINE_ROOT%\tools\w64devkit\bin\g++.exe
    exit /b 1
)

echo [INFO] Using compiler: %COMPILER%

:: Create build folder
if not exist "%PROJ_BUILD%" mkdir "%PROJ_BUILD%"
if not exist "%PROJ_BUILD%\Scenes" mkdir "%PROJ_BUILD%\Scenes"

:: Scan for user scripts in Content folder
set "USER_SCRIPTS="
set /a SCRIPT_COUNT=0
if exist "%PROJ_CONTENT%" (
    for /r "%PROJ_CONTENT%" %%s in (*.cpp) do (
        set "s_name=%%~nxs"
        set "s_path=%%s"
        echo "!s_path!" | findstr /i "\.assetmeta \.meta" >nul 2>&1
        if !ERRORLEVEL! neq 0 (
            set "USER_SCRIPTS=!USER_SCRIPTS! "%%s""
            set /a SCRIPT_COUNT+=1
            echo [INFO] Found script: %%~nxs
        )
    )
)

echo [INFO] Total behaviour scripts found: !SCRIPT_COUNT!

:: Terminate running game executable if active to prevent Windows file lock
taskkill /F /IM test.exe >nul 2>&1
taskkill /F /IM "%PROJ_NAME%.exe" >nul 2>&1

:: Compile standalone game binary including all user behaviour scripts
echo [INFO] Compiling behaviour scripts into %PROJ_BUILD%\%PROJ_NAME%.exe and test.exe...

"%COMPILER%" -std=c++20 -O2 ^
    "-I%ENGINE_ROOT%\Engine\EngineCore\Include" ^
    "-I%ENGINE_ROOT%\Engine\EnginePlatform\Include" ^
    "-I%ENGINE_ROOT%\Engine\EngineScene\Include" ^
    "-I%ENGINE_ROOT%\Engine\EngineAssets\Include" ^
    "-I%ENGINE_ROOT%\Engine\EngineRHI\Include" ^
    "-I%ENGINE_ROOT%\Engine\EngineRenderer\Include" ^
    "-I%ENGINE_ROOT%\Engine\EunoiaPluginCore\Include" ^
    "-I%ENGINE_ROOT%\deps\tinyobj" ^
    "-I%ENGINE_ROOT%\deps\cgltf" ^
    "-I%ENGINE_ROOT%\deps\ufbx" ^
    "-I%ENGINE_ROOT%\deps\glfw-3.5.1.bin.WIN64\include" ^
    "-I%ENGINE_ROOT%\deps\glm" ^
    "-I%ENGINE_ROOT%\deps\json" ^
    "-I%ENGINE_ROOT%" ^
    "%ENGINE_ROOT%\deps\ufbx\ufbx.c" ^
    "%ENGINE_ROOT%\Engine\EnginePlatform\Src\Window.cpp" ^
    "%ENGINE_ROOT%\Engine\EngineScene\Src\EunoiaBehaviour.cpp" ^
    "%ENGINE_ROOT%\Engine\EngineAssets\Src\AssetRegistry.cpp" ^
    "%ENGINE_ROOT%\Engine\EngineAssets\Src\AssetManager.cpp" ^
    "%ENGINE_ROOT%\Engine\EngineAssets\Src\TextureManager.cpp" ^
    "%ENGINE_ROOT%\Engine\EngineAssets\Src\MeshImporter.cpp" ^
    "%ENGINE_ROOT%\Runtime\Src\Main.cpp" ^
    !USER_SCRIPTS! ^
    "-L%ENGINE_ROOT%\deps\glfw-3.5.1.bin.WIN64\lib-mingw-w64" ^
    -lglfw3 -lgdi32 -lole32 -lshell32 ^
    -o "%PROJ_BUILD%\test.exe"

if !ERRORLEVEL! neq 0 (
    echo [ERROR] Script compilation failed. Please inspect the compiler errors above.
    exit /b !ERRORLEVEL!
)

:: Copy to project-named exe as well
copy /y "%PROJ_BUILD%\test.exe" "%PROJ_BUILD%\%PROJ_NAME%.exe" >nul 2>&1

:: Copy dependencies and runtime DLLs
if exist "%ENGINE_ROOT%\deps\glfw-3.5.1.bin.WIN64\lib-mingw-w64\glfw3.dll" (
    copy /y "%ENGINE_ROOT%\deps\glfw-3.5.1.bin.WIN64\lib-mingw-w64\glfw3.dll" "%PROJ_BUILD%\" >nul 2>&1
)
if exist "%ENGINE_ROOT%\Plugins\primitives\primitives.dll" (
    copy /y "%ENGINE_ROOT%\Plugins\primitives\primitives.dll" "%PROJ_BUILD%\" >nul 2>&1
)

:: Copy shaders and resources
if exist "%ENGINE_ROOT%\Shaders" xcopy /s /e /y /d /q /i "%ENGINE_ROOT%\Shaders" "%PROJ_BUILD%\Shaders\" >nul 2>&1
if exist "%ENGINE_ROOT%\Resources" xcopy /s /e /y /d /q /i "%ENGINE_ROOT%\Resources" "%PROJ_BUILD%\Resources\" >nul 2>&1

:: Copy project assets
if exist "%PROJ_PATH%\Cooked" xcopy /s /e /y /d /q /i "%PROJ_PATH%\Cooked" "%PROJ_BUILD%\Cooked\" >nul 2>&1
if exist "%PROJ_CONTENT%\Scenes" xcopy /s /e /y /d /q /i "%PROJ_CONTENT%\Scenes" "%PROJ_BUILD%\Scenes\" >nul 2>&1
if exist "%PROJ_PATH%\Scenes" xcopy /s /e /y /d /q /i "%PROJ_PATH%\Scenes" "%PROJ_BUILD%\Scenes\" >nul 2>&1
if exist "%PROJ_CONTENT%\Materials" xcopy /s /e /y /d /q /i "%PROJ_CONTENT%\Materials" "%PROJ_BUILD%\Materials\" >nul 2>&1
if exist "%PROJ_CONTENT%\Models" xcopy /s /e /y /d /q /i "%PROJ_CONTENT%\Models" "%PROJ_BUILD%\Models\" >nul 2>&1

:: Copy any loose scene files or metadata in Content
for %%f in ("%PROJ_CONTENT%\*.escene" "%PROJ_CONTENT%\*.emat" "%PROJ_CONTENT%\*.json") do (
    if exist "%%f" copy /y "%%f" "%PROJ_BUILD%\" >nul 2>&1
)

:: Write Run.bat launcher
(
echo @echo off
echo cd /d "%%~dp0"
echo echo [INFO] Starting %PROJ_NAME%...
echo start "" "test.exe"
) > "%PROJ_BUILD%\Run.bat"

echo [SUCCESS] Script compilation completed successfully!
echo [SUCCESS] Output binary: %PROJ_BUILD%\test.exe
exit /b 0
