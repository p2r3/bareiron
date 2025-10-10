# Bare-Iron Minecraft Server

**A minimalist, high-performance Minecraft server designed for memory-constrained embedded systems like the ESP32 and ESP8266.**

---

## About The Project

The goal of this project is to host a functional Minecraft server on extremely low-power devices. The server is written from scratch in C with a strong focus on efficiency, following this order of priorities: **1. Memory Usage**, **2. Performance**, **3. Features**.

Due to this focus, Bare-Iron is not a vanilla-compliant server. It is a lightweight re-implementation of the core Minecraft experience, designed to be as resource-friendly as possible.

- **Supported Minecraft Version:** `1.21.8`
- **Protocol Version:** `772`

> **Warning:** Currently, only the vanilla Minecraft client is officially supported. Using modded clients like Fabric or Forge may cause connection issues.

---

## Features

- **Cross-platform:** Runs on Windows, Linux, and macOS (x86_64).
- **Embedded Ready:** Optimized for ESP32 and ESP8266 microcontrollers.
- **Multi-Dimension Support:** Explore the Overworld, Nether, and End.
- **Dynamic World Generation:** Infinite terrain generation with basic biomes and structures.
- **Core Gameplay:** Survival mode with block breaking/placing, basic crafting, and mobs.

---

## Getting Started

### For PC (Windows, Linux, macOS)

The easiest way to get started on a PC is to download the latest pre-compiled binary from the [Releases](https://github.com/p2r3/bareiron/releases) page. The executable is a Cosmopolitan Polyglot, meaning the same file will run across different operating systems.

### For Microcontrollers (ESP32 & ESP8266)

This guide uses **PlatformIO** with Visual Studio Code for a streamlined experience.

#### Step 1: Environment Setup

1.  Install [Visual Studio Code](https://code.visualstudio.com/).
2.  Install the [PlatformIO IDE extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide) from the VS Code Marketplace.
3.  Create a new PlatformIO project:
    - Open the PlatformIO sidebar, navigate to "Projects & Configurations", and click "New Project".
    - Give your project a name (e.g., `bare-iron-server`).
    - Select the appropriate board:
      - **For ESP32:** `Espressif ESP32 Dev Module`
      - **For NodeMCU V3 (ESP8266):** `NodeMCU 1.0 (ESP-12E Module)`
    - For the **Framework**, you **must** select `Espressif IoT Development Framework` (ESP-IDF). Do not use the Arduino framework.
4.  Once the project is created, clone this repository into the project's root folder, overwriting any existing files. You can do this manually or with Git:
    ```bash
    git clone https://github.com/p2r3/bareiron.git .
    ```

#### Step 2: Board-Specific Configuration

Open the `platformio.ini` file in your project's root directory.

- **For ESP8266 (NodeMCU V3):**
  The ESP8266 is heavily resource-constrained. To enable the necessary memory optimizations, add the following build flag to your environment configuration:
  ```ini
  [env:nodemcuv2]
  platform = espressif8266
  board = nodemcuv2
  framework = esp-idf
  build_flags = -D TARGET_ESP8266
  ```

- **For ESP32:**
  The ESP32 is powerful enough to run the server with its default settings. No extra build flags are required.

#### Step 3: Server Configuration

1.  Open `include/globals.h`.
2.  Set your Wi-Fi credentials by modifying the `WIFI_SSID` and `WIFI_PASS` macros:
    ```c
    #define WIFI_SSID "your-ssid"
    #define WIFI_PASS "your-password"
    ```
3.  (Optional) You can explore this file to configure other gameplay settings like `GAMEMODE`.

#### Step 4: Build and Upload

1.  Connect your microcontroller to your computer.
2.  In the VS Code bottom toolbar, click the "Upload" button (an arrow icon). PlatformIO will compile the project and flash it to your device.
3.  After a successful upload, open the "Serial Monitor" to view the server's startup logs and find its assigned IP address.

---

## Advanced Configuration

For users who wish to fine-tune the server's performance and features, most high-level options are available in `include/globals.h`. Some key options include:

- `BROADCAST_ALL_MOVEMENT`: Disabling this ties player movement updates to the server tickrate, reducing network traffic at the cost of smoother movement.
- `ALLOW_CHESTS` & `DO_FLUID_FLOW`: These features can be disabled to save memory and processing power, which is recommended on highly constrained devices.
- `MAX_BLOCK_CHANGES`: Defines the maximum number of player-made block modifications stored in memory. Lowering this value significantly reduces RAM usage.

---

## Contribution

Contributions are welcome! Please create an issue to discuss your ideas before submitting a pull request. Adherence to the existing code style is required.

---

## License

Distributed under the MIT License. See `LICENSE` for more information.