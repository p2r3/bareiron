@echo off
setlocal enabledelayedexpansion
if not exist "%~dp0src\registries.h" (
    echo ERROR: "%~dp0src\registries.h" not found.
    echo Please prepare the correct registries.h file in the src folder.
    pause
    exit /b 1
)
set "files="
for /f "delims=" %%f in ('dir /b "%~dp0src\*.c" 2^>nul') do (
    set "files=!files! "%~dp0src%%f""
)
if "%files%"=="" (
    echo No C source files found in "%~dp0src".
    echo Add .c files to the src folder and try again.
    pause
    exit /b 1
)
if exist "%~dp0bareiron.exe" del /q "%~dp0bareiron.exe"
gcc %files% -o "%~dp0bareiron.exe"
if errorlevel 1 (
    echo Build failed.
    pause
    exit /b 1
) else (
    echo Build succeeded: "%~dp0bareiron.exe"
    pause
)