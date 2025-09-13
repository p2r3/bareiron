echo off
REM PREP REGISTRIES.H
del server.jar
rd /s /q notchian
md notchian
wget https://piston-data.mojang.com/v1/objects/6bce4ef400e4efaa63a13d5e6f6b500be969ef81/server.jar
move server.jar notchian
cd notchian
REM requires java 21 as default java
java -DbundlerMainClass="net.minecraft.data.Main" -jar server.jar --all
cd..
node build_registries.js
REM BUILD
gcc src/*.c -Iinclude -O3 -o bareiron.exe -lws2_32