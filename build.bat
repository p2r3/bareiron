@echo off
setlocal enabledelayedexpansion
set files=
for /f "usebackq delims=" %%f in (`dir /b "%cd%\src\*.c"`) do (
    set files=!files! src\%%f
)
if exist bareiron.exe del bareiron.exe
gcc %files% -o bareiron
pause
