#include <stdio.h>
#include <string.h>
#include "worldgen.h"
#include "worldgen_v2.h"
#include "biome_config.h"

// Adapter to use V2 worldgen with V1 interface
// This maintains compatibility with existing code

static int use_v2_worldgen = 0;

void init_worldgen_system(const char *config_file) {
  if (load_world_config(config_file)) {
    use_v2_worldgen = 1;
    printf("✓ Enhanced worldgen v2 enabled\n");
  } else {
    use_v2_worldgen = 0;
    printf("✗ Using legacy worldgen (v2 config not loaded)\n");
  }
}

// Wrapper that automatically uses V2 if available, V1 otherwise
uint8_t buildChunkSection_auto(int cx, int cy, int cz) {
  if (use_v2_worldgen) {
    // Use V2 (writes to chunk_section global automatically)
    return buildChunkSectionV2(cx, cy, cz, chunk_section);
  } else {
    // Use legacy V1
    return buildChunkSection(cx, cy, cz);
  }
}

int is_using_v2_worldgen(void) {
  return use_v2_worldgen;
}
