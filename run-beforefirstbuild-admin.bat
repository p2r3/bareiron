echo off
REM GET REQUIRED PROGRAMS
winget install --id=MSYS2.MSYS2
winget install nodejs
winget install wge
C:\msys64\ucrt64.exe pacman -S mingw-w64-ucrt-x86_64-gcc --noconfirm"
echo CHERE_INVOKING=1 >> C:\msys64\ucrt64.ini