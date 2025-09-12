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
- To target an ESP variant, set up a PlatformIO project and clone this repository on top of it. See **Configuration** below for further steps.
- There's currently no streamlined build process for Windows. Contributions in this area are welcome!

## Configuration
Configuring the server requires compiling it from its source code as described in the section above.

Most user-friendly configuration options are available in `include/globals.h`, including WiFi credentials for embedded setups. Some other details, like the MOTD or starting time of day, can be found in `src/globals.c`. For everything else, you'll have to dig through the code.

Here's a summary of some of the more important yet less trivial options for those who plan to use this on a real microcontroller with real players:

- Depending on the player count, the performance of the MCU, and the bandwidth of your network, player position broadcasting could potentially throttle your connection. If you find this to be the case, try commenting out `BROADCAST_ALL_MOVEMENT` and `SCALE_MOVEMENT_UPDATES_TO_PLAYER_COUNT`. This will tie movement to the tickrate. If this change makes movement too choppy, you can decrease `TIME_BETWEEN_TICKS` at the cost of more compute.
- If you experience crashes or instability related to chests or water, those features can be disabled with `ALLOW_CHESTS` and `DO_FLUID_FLOW`, respectively.
- If you find frequent repeated chunk generation to choke the server, increasing `VISITED_HISTORY` might help. There isn't _that_ much of a memory footprint for this - increasing it to `64` for example would only take up 240 extra bytes per allocated player.

## Non-volatile storage (optional)
This section applies to those who target ESP variants and wish to persist world data after a shutdown. *This is not necessary on PC platforms*, as world and player data is written to `world.bin` by default.

The simplest way to accomplish this is to set up LittleFS in PlatformIO and comment out the `#ifndef` surrounding `SYNC_WORLD_TO_DISK` in `globals.h`. Since flash writes are typically slow and blocking, you'll likely want to uncomment `DISK_SYNC_BLOCKS_ON_INTERVAL`. Depending on the flash size of your board, you may also have to decrease `MAX_BLOCK_CHANGES`, so that the world data fits in your LittleFS partition.

If using an SD card module or other virtual file system, you'll have to implement the filesystem setup routine on your own. The built-in serializer should still work though, as it uses POSIX filesystem calls.

Alternatively, if you can't set up a file system, you can dump and upload world data over TCP. This can be enabled by uncommenting `DEV_ENABLE_BEEF_DUMPS` in `globals.h`. *Note: this system implements no security or authentication.* With this option enabled, anyone with access to the server can upload arbitrary world data.

## Contribution
- Create issues and discuss with the maintainer(s) before making pull requests.
- Follow the existing code style. Ensure that your changes fit in with the surrounding code, even if you disagree with the style. Pull requests with inconsistent style will be nitpicked.

## How to run the server on Debian and Ubuntu

Case n°1 - You simply want to run the server temporarily on a Debian / Ubuntu remove server

Apps required :

- wget ( `sudo apt install wget` )

Commands you will have to use :

1 - Download the bareiron file

- `wget https://github.com/p2r3/bareiron/releases/download/latest/bareiron.exe` ( will download the bareiron.exe file inside the folder where you currently are )

2 - Grant your Debian / Ubuntu device the permissions to run the bareiron.exe file

- `chmod +x ./bareiron.exe` ( will make the bareiron.exe file a file you can run on your Debian / Ubuntu device, only required after you downloaded the file, once it's done you don't have to do it anymore )

3 - Run the bareiron.exe file :

- `./bareiron.exe`


Case n°2 - You want the minecraft server to run on Debian / Ubuntu startup :

Apps required :

- wget ( `sudo apt install wget` )
- nano ( `sudo apt install nano` )

Commands you will have to use :

1 - Create the service file into the systemd system folder

- `nano /etc/systemd/system/mc-server.service`

2 - Write the service file content ( you can use the one i made )

Paste the following :
`[Unit]
Description=Minecraft Server
After=network.target

[Service]
ExecStart=chmod +x /home/bareiron.exe && /home/bareiron.exe
Restart=always
WorkingDirectory=/home/

[Install]
WantedBy=multi-user.target`

3 - Grant your Debian / Ubuntu device the permissions to run the bareiron.exe file

- `chmod +x ./mc-server.service`

4 - Start the service

- `sudo systemctl start mc-server.service`

5 - Verify if the minecraft server is started successfully

- `sudo systemctl status mc-server.service`

Once you have verified than your Minecraft server is successfully running and isn't throwing errors, use CTRL + C on your computer to leave the logs of the service or directly close the console


Case n°3 - You want to build the Minecraft server by yourself :

Apps required :

- git ( `sudo apt install git` )
- gcc ( `sudo apt install gcc` )

Commands you will have to run :

1 - Get all the repository files

- `git clone https://github.com/p2r3/bareiron.git`

2 - Get into the bareiron folder

- `cd ./bareiron`

3 - Grant your Debian / Ubuntu device the permissions to run the build.sh file

- `chmod +x ./build.sh`

4 - Run the build.sh file

- `./build.sh`
