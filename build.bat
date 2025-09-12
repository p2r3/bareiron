@echo off
setlocal enabledelayedexpansion

if not exist "include\registries.h" (
    echo Error: 'include/registries.h' is missing.
    echo Please follow the 'Compilation' section of the README to generate it.
    pause
	exit /b 1
)

set "files="
for %%f in (src\*.c) do (
    set "files=!files! %%f"
)

if "%files%"=="" (
    echo No C source files found in "src".
	pause
    exit /b 1
)

if exist "bareiron.exe" del /q "bareiron.exe"

gcc src\*.c -O3 -Iinclude -o bareiron.exe
if errorlevel 1 (
    echo Build failed.
    pause
    exit /b 1
) else (
    echo Build succeeded: "bareiron.exe"
    pause
)