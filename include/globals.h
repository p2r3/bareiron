#ifndef H_GLOBALS
#define H_GLOBALS

#include <stdint.h>
#include <unistd.h>

#include "registries.h"

#ifdef ESP_PLATFORM
  #define WIFI_SSID "your-ssid"
  #define WIFI_PASS "your-password"
  void task_yield ();
#else
  #define task_yield();
#endif

#define true 1
#define false 0

// TCP port, Minecraft's default is 25565
#define PORT 25565

// How many players to keep in memory, NOT the amount of concurrent players
// Even when offline, players who have logged on before take up a slot
#define MAX_PLAYERS 16

// How many mobs to allocate memory for
#define MAX_MOBS (MAX_PLAYERS)

// Manhattan distance at which mobs despawn
#define MOB_DESPAWN_DISTANCE 256

// Server game mode: 0 - survival; 1 - creative; 2 - adventure; 3 - spectator
#define GAMEMODE 0

// Max render distance, determines how many chunks to send
#define VIEW_DISTANCE 2

// Time between server ticks in microseconds (default = 1s)
#define TIME_BETWEEN_TICKS 1000000

// Calculated from TIME_BETWEEN_TICKS
#define TICKS_PER_SECOND ((float)1000000 / TIME_BETWEEN_TICKS)

// Initial world generation seed, will be hashed on startup
// Used in generating terrain and biomes
#define INITIAL_WORLD_SEED 0xA103DE6C

// Initial general RNG seed, will be hashed on startup
// Used in random game events like item drops and mob behavior
#define INITIAL_RNG_SEED 0xE2B9419

// Size of each bilinearly interpolated area ("minichunk")
// For best performance, CHUNK_SIZE should be a power of 2
#define CHUNK_SIZE 8

// Terrain low point - should start a bit below sea level for rivers/lakes
#define TERRAIN_BASE_HEIGHT 60

// Cave generation Y level
#define CAVE_BASE_DEPTH 24

// Size of every major biome in multiples of CHUNK_SIZE
// For best performance, should also be a power of 2
#define BIOME_SIZE (CHUNK_SIZE * 8)

// Calculated from BIOME_SIZE
#define BIOME_RADIUS (BIOME_SIZE / 2)

// How many visited chunk coordinates to "remember"
// The server will not re-send chunks that the player has recently been in
// Must be at least 1, otherwise chunks will be sent on each position update
#define VISITED_HISTORY 4

// How many player-made block changes to allow
// Determines the fixed amount of memory allocated to blocks
#define MAX_BLOCK_CHANGES 20000

// If defined, writes and reads world data to/from disk (or flash).
// This is a synchronous operation, and can cause performance issues if
// frequent random disk access is slow. Data is still stored in and
// accessed from memory - reading from disk is only done on startup.
// When targeting ESP-IDF, LittleFS is used to manage flash reads and
// writes. Flash is typically *very* slow and unreliable, which is why
// this option is disabled by default when targeting ESP-IDF.
#ifndef ESP_PLATFORM
  #define SYNC_WORLD_TO_DISK
#endif

// The minimum interval (in microseconds) at which certain data is written
// to disk/flash. Bounded on the low end by TIME_BETWEEN_TICKS. By default,
// applies only to player data. Block changes are written as soon as they
// are made, but in much smaller portions. Set DISK_SYNC_BLOCKS_ON_INTERVAL
// to make this apply to block changes as well.
#define DISK_SYNC_INTERVAL 15000000

// Whether to sync block changes to disk on an interval, instead of syncing
// on each change. On systems with fast random disk access, this shouldn't
// be necessary.
// #define DISK_SYNC_BLOCKS_ON_INTERVAL

// Time in microseconds to spend waiting for data transmission before
// timing out. Default is 15s, which leaves 5s to prevent starving other
// clients from Keep Alive packets.
#define NETWORK_TIMEOUT_TIME 15000000

// Size of the receive buffer for incoming string data
#define MAX_RECV_BUF_LEN 256

// Tools that should have a durability decrease when breaking a block
#define TOOLS { \
    I_wooden_pickaxe, I_wooden_axe, I_wooden_shovel, \
    I_stone_pickaxe,  I_stone_axe,  I_stone_shovel, \
    I_iron_pickaxe,   I_iron_axe,   I_iron_shovel, \
    I_golden_pickaxe, I_golden_axe, I_golden_shovel, \
    I_diamond_pickaxe,I_diamond_axe,I_diamond_shovel, \
    I_netherite_pickaxe, I_netherite_axe, I_netherite_shovel, \
    I_shears \
}

// The durability of each tool
#define TOOL_DURABILITY { \
    59, 59, 59, \
    131, 131, 131, \
    250, 250, 250, \
    32, 32, 32, \
    1561, 1561, 1561, \
    2031, 2031, 2031, \
    238 \
}

// You must to ensure this after changing the options ahead!!!
#define TOOL_COUNT 19

// If defined, sends the server brand to clients. Doesn't do much, but will
// show up in the top-left of the F3/debug menu, in the Minecraft client.
// You can change the brand string in the "brand" variable in src/globals.c
#define SEND_BRAND

// If defined, rebroadcasts ALL incoming movement updates, disconnecting
// movement from the server's tickrate. This makes movement much smoother
// on very low tickrates, at the cost of potential network instability when
// hosting more than just a couple of players. When disabling this on low
// tickrates, consider disabling SCALE_MOVEMENT_UPDATES_TO_PLAYER_COUNT too.
#define BROADCAST_ALL_MOVEMENT

// If defined, scales the frequency at which player movement updates are
// broadcast based on the amount of players, reducing overhead for higher
// player counts. For very many players, makes movement look jittery.
// It is not recommended to use this if BROADCAST_ALL_MOVEMENT is disabled
// on low tickrates, as that might drastically decrease the update rate.
#define SCALE_MOVEMENT_UPDATES_TO_PLAYER_COUNT

// If defined, calculates fluid flow when blocks are updated near fluids
// Somewhat computationally expensive and potentially unstable
#define DO_FLUID_FLOW

// If defined, allows players to craft and use chests.
// Chests take up 15 block change slots each, require additional checks,
// and use some terrible memory hacks to function. On some platforms, this
// could cause bad performance or even crashes during gameplay.
#define ALLOW_CHESTS

// If defined, enables flight for all players. As a side-effect, allows
// players to sprint when starving.
// #define ENABLE_PLAYER_FLIGHT

// If defined, enables the item pickup animation when mining a block/
// Does not affect how item pickups work! Items from broken blocks still
// get placed directly in the inventory, this is just an animation.
// Relatively inexpensive, though requires sending a few more packets
// every time a block is broken.
#define ENABLE_PICKUP_ANIMATION

// If defined, players are able to receive damage from nearby cacti.
#define ENABLE_CACTUS_DAMAGE

// If defined, logs unrecognized packet IDs
// #define DEV_LOG_UNKNOWN_PACKETS

// If defined, logs cases when packet length doesn't match parsed byte count
#define DEV_LOG_LENGTH_DISCREPANCY

// If defined, log chunk generation events
// #define DEV_LOG_CHUNK_GENERATION

// If defined, allows dumping world data by sending 0xBEEF (big-endian),
// and uploading world data by sending 0xFEED, followed by the data buffer.
// Doesn't implement authentication, hence disabled by default.
// #define DEV_ENABLE_BEEF_DUMPS

#define STATE_NONE 0
#define STATE_STATUS 1
#define STATE_LOGIN 2
#define STATE_TRANSFER 3
#define STATE_CONFIGURATION 4
#define STATE_PLAY 5

extern ssize_t recv_count;
extern uint8_t recv_buffer[MAX_RECV_BUF_LEN];

extern uint32_t world_seed;
extern uint32_t rng_seed;

extern uint16_t world_time;
extern uint32_t server_ticks;

extern char motd[];
extern uint8_t motd_len;

#ifdef SEND_BRAND
  extern char brand[];
  extern uint8_t brand_len;
#endif

extern uint16_t client_count;

typedef struct {
  short x;
  short z;
  uint8_t y;
  uint8_t block;
} BlockChange;

#pragma pack(push, 1)

typedef struct {
  uint8_t uuid[16];
  char name[16];
  int client_fd;
  short x;
  uint8_t y;
  short z;
  short visited_x[VISITED_HISTORY];
  short visited_z[VISITED_HISTORY];
  #ifdef SCALE_MOVEMENT_UPDATES_TO_PLAYER_COUNT
    uint16_t packets_since_update;
  #endif
  int8_t yaw;
  int8_t pitch;
  uint8_t grounded_y;
  uint8_t health;
  uint8_t hunger;
  uint16_t saturation;
  uint8_t hotbar;
  uint16_t inventory_items[41];
  uint16_t craft_items[9];
  uint8_t inventory_count[41];
  uint8_t craft_count[9];
  // Usage depends on player's flags, see below
  // When no flags are set, acts as cursor item ID
  uint16_t flagval_16;
  // Usage depends on player's flags, see below
  // When no flags are set, acts as cursor item count
  uint8_t flagval_8;
  // 0x01 - attack cooldown, uses flagval_8 as the timer
  // 0x02 - has not spawned yet
  // 0x04 - sneaking
  // 0x08 - sprinting
  // 0x10 - eating, makes flagval_16 act as eating timer
  // 0x20 - client loading, uses flagval_16 as fallback timer
  // 0x40 - movement update cooldown
  // 0x80 - craft_items lock (for storing pointers)
  uint8_t flags;
} PlayerData;

typedef struct {
  uint8_t type;
  short x;
  // When the mob is dead (health is 0), the Y coordinate acts
  // as a timer for deleting and deallocating the mob
  uint8_t y;
  short z;
  // Lower 5 bits: health
  // Middle 1 bit: sheep sheared, unused for other mobs
  // Upper 2 bits: panic timer
  uint8_t data;
} MobData;

#pragma pack(pop)

union EntityDataValue {
  uint8_t byte;
  int pose;
};

typedef struct {
  uint8_t index;
  // 0 - Byte
  // 21 - Pose
  int type;
  union EntityDataValue value;
} EntityData;

extern BlockChange block_changes[MAX_BLOCK_CHANGES];
extern int block_changes_count;

extern PlayerData player_data[MAX_PLAYERS];
extern int player_data_count;

extern MobData mob_data[MAX_MOBS];

extern uint16_t tools[];
extern uint16_t tool_durability[];

#endif
