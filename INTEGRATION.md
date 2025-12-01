# Quick Integration Guide

## Minimal Changes to Enable V2 Worldgen

### Step 1: Add Includes to `src/main.c`

Add these includes at the top (after existing includes):

```c
#include "biome_config.h"
#include "threading.h"
#include "worldgen_adapter.h"
```

### Step 2: Initialize Worldgen System

In `main()` function, after the world seed initialization (around line 517), add:

```c
// Initialize enhanced worldgen system
init_worldgen_system("biomes.json");

// If V2 is enabled, initialize thread pool
if (is_using_v2_worldgen()) {
  if (!init_chunk_pool(world_config.thread_count)) {
    fprintf(stderr, "Failed to initialize chunk thread pool\n");
    // Fall back to single-threaded V2
  }
}
```

### Step 3: Use Auto-Switching Worldgen (OPTIONAL)

In `src/packets.c`, replace:

```c
uint8_t biome = buildChunkSection(x, y, z);
```

With:

```c
uint8_t biome = buildChunkSection_auto(x, y, z);
```

(And add `#include "worldgen_adapter.h"` to packets.c)

### Step 4: Cleanup on Exit (OPTIONAL)

At the end of `main()`, before return, add:

```c
if (is_using_v2_worldgen()) {
  shutdown_chunk_pool();
  free_world_config();
}
```

## That's It!

The system will:
- Try to load `biomes.json`
- If successful → Use enhanced V2 worldgen with multithreading
- If failed → Fall back to legacy V1 worldgen (no changes to behavior)

## Testing

1. **Without biomes.json**: Server runs normally with legacy worldgen
2. **With biomes.json**: Server uses new JSON-configured worldgen

```bash
# Build
./build.sh

# Run (should automatically detect biomes.json)
./bareiron
```

## Full Integration Example for main.c

Here's the complete patch for `src/main.c`:

```c
// Add after existing includes:
#include "biome_config.h"
#include "threading.h"
#include "worldgen_adapter.h"

int main () {
  #ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
      fprintf(stderr, "WSAStartup failed\n");
      exit(EXIT_FAILURE);
    }
  #endif

  // Hash the seeds to ensure they're random enough
  world_seed = splitmix64(world_seed);
  printf("World seed (hashed): ");
  for (int i = 3; i >= 0; i --) printf("%X", (unsigned int)((world_seed >> (8 * i)) & 255));

  rng_seed = splitmix64(rng_seed);
  printf("\nRNG seed (hashed): ");
  for (int i = 3; i >= 0; i --) printf("%X", (unsigned int)((rng_seed >> (8 * i)) & 255));
  printf("\n\n");

  // ===== ADD THIS BLOCK =====
  // Initialize enhanced worldgen system
  printf("Initializing world generation...\n");
  init_worldgen_system("biomes.json");

  if (is_using_v2_worldgen()) {
    printf("Starting chunk generation thread pool...\n");
    if (!init_chunk_pool(world_config.thread_count)) {
      fprintf(stderr, "Warning: Failed to initialize thread pool\n");
    }
  }
  printf("\n");
  // ===== END OF ADDITIONS =====

  // ... rest of existing code ...
}
```

## Advanced: Multithreaded Chunk Generation

For fully async chunk generation, modify chunk sending in `packets.c`:

### Current (Synchronous):
```c
uint8_t biome = buildChunkSection(x, y, z);
send_all(client_fd, chunk_section, 4096);
```

### Enhanced (Async - TODO for future):
```c
// Submit request (non-blocking)
float priority = player_distance_to_chunk(x, z);
uint64_t req = request_chunk_generation(x, y, z, priority);

// In main loop, poll for completed chunks
GeneratedChunk completed[32];
int count = poll_completed_chunks(completed, 32);
for (int i = 0; i < count; i++) {
  send_chunk_to_player(&completed[i]);
}
```

For now, the adapter provides thread-safety while keeping the synchronous interface.
