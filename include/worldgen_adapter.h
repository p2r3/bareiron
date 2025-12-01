#ifndef H_WORLDGEN_ADAPTER
#define H_WORLDGEN_ADAPTER

#include <stdint.h>

// Initialize worldgen system from config file
// If config loads successfully, V2 worldgen will be used
// Otherwise, falls back to legacy V1 worldgen
void init_worldgen_system(const char *config_file);

// Drop-in replacement for buildChunkSection()
// Automatically uses V2 if config is loaded, otherwise uses V1
uint8_t buildChunkSection_auto(int cx, int cy, int cz);

// Check which worldgen is being used
int is_using_v2_worldgen(void);

#endif
