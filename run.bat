@echo off
cd /d "%~dp0"
if not exist "Eunoia-Editor.exe" (
    echo [INFO] Eunoia-Editor.exe not found. Building first...
    call build.bat
    if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
)

echo [INFO] Starting Eunoia-Editor (DirectX 12)...
start "" "Eunoia-Editor.exe"
