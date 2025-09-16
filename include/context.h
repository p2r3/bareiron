#ifndef H_CONTEXT
#define H_CONTEXT

#include "globals.h"

// Central server context replacing scattered globals.
// This definition must stay in sync with Zig's extern ServerContext.
typedef struct ServerContext {
  // Seeds and timing
  uint32_t world_seed;
  uint32_t rng_seed;
  uint16_t world_time;
  uint32_t server_ticks;

  // Connections
  uint16_t client_count;

  // World state
  BlockChange block_changes[MAX_BLOCK_CHANGES];
  int block_changes_count;

  // Players
  PlayerData player_data[MAX_PLAYERS];
  int player_data_count;

  // Mobs
  MobData mob_data[MAX_MOBS];

  // Branding/MOTD (length + fixed-capacity buffers)
  uint8_t motd_len;
  char motd[64];
  #ifdef SEND_BRAND
    uint8_t brand_len;
    char brand[32];
  #endif
} ServerContext;

#endif
