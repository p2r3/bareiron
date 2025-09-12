# bareiron
Minimalist Minecraft server for memory-restrictive embedded systems.

The goal of this project is to enable hosting Minecraft servers on very weak devices, such as the ESP32. The project's priorities are, in order: **memory usage**, **performance**, and **features**. Because of this, compliance with vanilla Minecraft is not guaranteed, nor is it a goal of the project.

- Minecraft version: `1.21.8`
- Protocol version: `772`

## Quick start
For PC x86_64 platforms, grab the [latest build binary](https://github.com/p2r3/bareiron/releases/download/latest/bareiron.exe) and run it. The file is a [Cosmopolitan polyglot](https://github.com/jart/cosmopolitan), which means it'll run on Windows, Linux, and possibly Mac, despite the file extension. Note that the server's default settings cannot be reconfigured without compiling from source.

For microcontrollers, see the section on **compilation** below.

## Compilation
Before compiling, you'll need to dump registry data from a vanilla Minecraft server. Create a folder called `notchian` here, and put a Minecraft server JAR in it. Then, follow [this guide](https://minecraft.wiki/w/Minecraft_Wiki:Projects/wiki.vg_merge/Data_Generators) to dump all of the registries. Finally, run `build_registries.js` with `node`, `bun`, or `deno`.

- To target Linux, install `gcc` and run `build.sh`
- To target Windows, install `gcc` and run `build.bat`
- To target an ESP variant, set up a PlatformIO project and clone this repository on top of it. Set your WiFi credentials in `include/globals.h`.

## Configuration
Configuring the server requires compiling it from its source code (see section above). 

Most user-friendly configuration options are available in `include/globals.h`, including WiFi credentials for embedded setups. Some other details, like the MOTD or starting time of day, can be found in `src/globals.c`. For everything else, you'll have to dig through the code.

## Non-volatile storage (optional)
This section applies to those who target ESP variants and wish to persist world data after a shutdown. *This is not necessary on PC platforms*, as world and player data is written to `world.bin` by default.

The simplest way to accomplish this is to set up LittleFS in PlatformIO and comment out the `#ifndef` surrounding `SYNC_WORLD_TO_DISK` in `globals.h`. Since flash writes are typically slow and blocking, you'll likely want to uncomment `DISK_SYNC_BLOCKS_ON_INTERVAL`. Depending on the flash size of your board, you may also have to decrease `MAX_BLOCK_CHANGES`, so that the world data fits in your LittleFS partition.

If using an SD card module or other virtual file system, you'll have to implement the filesystem setup routine on your own. The built-in serializer should still work though, as it uses POSIX filesystem calls.

Alternatively, if you can't set up a file system, you can dump and upload world data over TCP. This can be enabled by uncommenting `DEV_ENABLE_BEEF_DUMPS` in `globals.h`. *Note: this system implements no security or authentication.* With this option enabled, anyone with access to the server can upload arbitrary world data.

## Contribution
- Create issues and discuss with the maintainer(s) before making pull requests.
- Follow the existing code style. Ensure that your changes fit in with the surrounding code, even if you disagree with the style. Pull requests with inconsistent style will be nitpicked.
