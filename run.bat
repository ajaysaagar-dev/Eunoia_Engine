@echo off
setlocal
cd /d "%~dp0"

if not exist "EngineBuild\Eunoia-Editor.exe" (
    if not exist "Eunoia-Editor.exe" (
        echo [INFO] Eunoia-Editor.exe not found. Building first...
        call build.bat
        if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
    )
)

if exist "EngineBuild\Eunoia-Editor.exe" (
    echo [INFO] Starting Eunoia-Editor from EngineBuild...
    cd /d "%~dp0EngineBuild"
    start "" "Eunoia-Editor.exe"
) else (
    echo [INFO] Starting Eunoia-Editor...
    start "" "Eunoia-Editor.exe"
)
