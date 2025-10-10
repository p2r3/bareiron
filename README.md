# bareiron
Minimalist Minecraft server for memory-restrictive embedded systems.

The goal of this project is to enable hosting Minecraft servers on very weak devices, such as the ESP32. The project's priorities are, in order: **memory usage**, **performance**, and **features**. Because of this, compliance with vanilla Minecraft is not guaranteed, nor is it a goal of the project.

- Minecraft version: `1.21.8`
- Protocol version: `772`

> [!WARNING]
> Currently, only the vanilla client is officially supported. Issues have been reported when using Fabric or similar.

## Quick start
For PC x86_64 platforms, grab the [latest build binary](https://github.com/p2r3/bareiron/releases/download/latest/bareiron.exe) and run it. The file is a [Cosmopolitan polyglot](https://github.com/jart/cosmopolitan), which means it'll run on Windows, Linux, and possibly Mac, despite the file extension. Note that the server's default settings cannot be reconfigured without compiling from source.

For microcontrollers, see the section on **compilation** below.

## Compilation
Before compiling, you'll need to dump registry data from a vanilla Minecraft server. On Linux, this can be done automatically using the `extract_registries.sh` script. Otherwise, the manual process is as follows: create a folder called `notchian` here, and put a Minecraft server JAR in it. Then, follow [this guide](https://minecraft.wiki/w/Minecraft_Wiki:Projects/wiki.vg_merge/Data_Generators) to dump all of the registries (use the _second_ command with the `--all` flag). Finally, run `build_registries.js` with either [bun](https://bun.sh/), [node](https://nodejs.org/en/download), or [deno](https://docs.deno.com/runtime/getting_started/installation/).

- To compile on Linux, install `gcc` and run `./build.sh`.
- For compiling on Windows, there are a few options:
  - To compile a native Windows binary: install [MSYS2](https://www.msys2.org/) and open the "MSYS2 MINGW64" shell. From there, run `pacman -Sy mingw-w64-x86_64-gcc`, navigate to this project's directory, and run `./build.sh`.
  - To compile a native 32-bit binary (compatible with Windows 95/98, but why would you ever want that), use the same steps above, except with `pacman -Sy mingw-w64-cross-gcc` and `./build.sh --9x`.
  - To compile a MSYS2-linked binary: install [MSYS2](https://www.msys2.org/), and open the "MSYS2 MSYS" shell. From there, install `gcc` (run `pacman -Sy gcc`), navigate to this project's directory and run `./build.sh`. 
  - To compile and run a Linux binary from Windows: install WSL, and from there install `gcc` and run `./build.sh` in this project's directory.
- To target an ESP variant, set up a PlatformIO project (select the ESP-IDF framework, **not Arduino**) and clone this repository on top of it. See **Uruchomienie na mikrokontrolerach** below for detailed instructions.

Ta sekcja opisuje, jak skompilować i wgrać serwer na popularne płytki ESP32 i ESP8266 przy użyciu [PlatformIO](https://platformio.org/).

### Krok 1: Przygotowanie PlatformIO

1.  Zainstaluj [Visual Studio Code](https://code.visualstudio.com/).
2.  Zainstaluj rozszerzenie [PlatformIO IDE](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide) z marketu VSCode.
3.  Utwórz nowy projekt w PlatformIO:
    *   Kliknij ikonę PlatformIO na pasku bocznym.
    *   Wybierz "Projects & Configuration", a następnie "New Project".
    *   Nazwij projekt (np. `bareiron-esp`), a następnie wybierz odpowiednią płytkę:
        *   **Dla ESP32:** `Espressif ESP32 Dev Module`
        *   **Dla NodeMCU V3 (ESP8266):** `NodeMCU 1.0 (ESP-12E Module)`
    *   Jako **Framework** wybierz `Espressif IoT Development Framework` (ESP-IDF). **Nie używaj frameworka Arduino!**
4.  Po utworzeniu projektu, sklonuj to repozytorium do folderu projektu, nadpisując istniejące pliki. Możesz to zrobić ręcznie lub użyć Gita:
    ```bash
    git clone https://github.com/p2r3/bareiron.git .
    ```

### Krok 2: Konfiguracja specyficzna dla płytki

#### Dla NodeMCU V3 (ESP8266)

ESP8266 ma bardzo ograniczoną ilość pamięci RAM. Aby serwer mógł na nim działać, konieczne jest włączenie specjalnych optymalizacji.

1.  Otwórz plik `platformio.ini` w głównym folderze projektu.
2.  Dodaj następującą linię do sekcji `[env:nodemcuv2]`, aby zdefiniować flagę `TARGET_ESP8266`:
    ```ini
    build_flags = -D TARGET_ESP8266
    ```
    Ta flaga automatycznie zmniejszy zużycie pamięci i wyłączy niektóre funkcje.

#### Dla ESP32

ESP32 jest znacznie mocniejszy i nie wymaga dodatkowych flag kompilacji. Domyślne ustawienia powinny działać poprawnie.

### Krok 3: Konfiguracja serwera

1.  Otwórz plik `include/globals.h`.
2.  Znajdź i zmień `WIFI_SSID` oraz `WIFI_PASS` na swoje dane logowania do sieci Wi-Fi:
    ```c
    #define WIFI_SSID "twoje-ssid"
    #define WIFI_PASS "twoje-haslo"
    ```
3.  (Opcjonalnie) Możesz dostosować inne ustawienia serwera w tym samym pliku, takie jak `GAMEMODE` czy `TIME_BETWEEN_TICKS`.

### Krok 4: Kompilacja i wgranie

1.  Podłącz swoją płytkę ESP do komputera.
2.  W dolnym pasku narzędzi VSCode kliknij przycisk "Upload" (ikona strzałki w prawo). PlatformIO automatycznie skompiluje projekt i wgra go na urządzenie.
3.  Po zakończeniu wgrywania, otwórz monitor portu szeregowego ("Serial Monitor"), aby zobaczyć adres IP serwera.

## Konfiguracja zaawansowana
Poniższe opcje są przeznaczone dla zaawansowanych użytkowników, którzy chcą dostosować działanie serwera poza podstawową konfiguracją opisaną w samouczku.

Most user-friendly configuration options are available in `include/globals.h`. Some other details, like the MOTD or starting time of day, can be found in `src/globals.c`. For everything else, you'll have to dig through the code.

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
- Create issues and discuss with the maintainer(s) before making pull requests. Even for small changes.
- Follow the existing code style. Ensure that your changes fit in with the surrounding code, even if you disagree with the style. Pull requests with inconsistent style will be nitpicked.
- Test your code before creating a pull request or requesting a review, regardless of how "simple" your change is. It's a basic form of respect towards the maintainer and reviewer.
- Development tooling and compilation improvements _are not welcome,_ unless you've worked with the codebase long enough to have noticed practical shortcomings in that area. Adding a single compiler flag is not a meaningful first contribution.
- For information on the Minecraft server protocol, [refer to the wiki](https://minecraft.wiki/w/Java_Edition_protocol/Packets). For everything else, use a [search engine](https://google.com).

## Rozwiązywanie problemów (Troubleshooting)

### Problemy z pamięcią na ESP8266

*   **Błąd: `region `dram0_0_seg' overflows by ... bytes`**: Oznacza to, że program jest zbyt duży, aby zmieścić się w pamięci RAM urządzenia.
    *   **Rozwiązanie**: Upewnij się, że włączyłeś flagę `-D TARGET_ESP8266` w pliku `platformio.ini`. Jeśli problem nadal występuje, spróbuj jeszcze bardziej zmniejszyć wartości `MAX_PLAYERS` i `MAX_BLOCK_CHANGES` w pliku `include/globals.h`.

*   **Serwer restartuje się w pętli (Guru Meditation Error)**: Może to być spowodowane przepełnieniem stosu lub innym błędem krytycznym.
    *   **Rozwiązanie**: Spróbuj zmniejszyć rozmiar stosu dla zadania `bareiron_main` w pliku `src/main.c`. Znajdź linię `xTaskCreate(bareiron_main, "bareiron", 4096, ...)` i zmień `4096` na mniejszą wartość, np. `3072`.

### Problemy z kompilacją

*   **Błąd: `xtensa-lx106-elf-gcc: command not found`**: Oznacza to, że toolchain dla ESP8266 nie jest poprawnie zainstalowany.
    *   **Rozwiązanie**: Upewnij się, że PlatformIO poprawnie zainstalowało wszystkie zależności. Spróbuj usunąć folder `.platformio` w katalogu domowym, aby wymusić ponowną instalację.

### Problemy z siecią

*   **Serwer nie pojawia się w sieci**:
    *   **Rozwiązanie**: Sprawdź, czy dane logowania Wi-Fi w `include/globals.h` są poprawne. Otwórz monitor portu szeregowego, aby sprawdzić, czy urządzenie pomyślnie połączyło się z siecią i jaki otrzymało adres IP.