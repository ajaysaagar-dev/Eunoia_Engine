@echo off
setlocal
cd /d "%~dp0"

if not exist "Build\Engine\Eunoia-Editor.exe" (
    echo [INFO] Build\Engine\Eunoia-Editor.exe not found. Building first...
    call Build.bat
    if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
)

if exist "Build\Engine\Eunoia-Editor.exe" (
    echo [INFO] Starting Eunoia-Editor from Build\Engine...
    cd /d "%~dp0Build\Engine"
    start "" "Eunoia-Editor.exe"
) else (
    echo [ERROR] Could not find or build Eunoia-Editor.exe.
    exit /b 1
)
