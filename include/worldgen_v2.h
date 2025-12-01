#ifndef H_WORLDGEN_V2
#define H_WORLDGEN_V2

#include <stdint.h>
#include "biome_config.h"
#include "noise.h"

// Enhanced chunk section builder using JSON configuration
// Returns biome ID at origin corner
uint8_t buildChunkSectionV2(int cx, int cy, int cz, uint8_t *output);

// Get terrain height at coordinates using configured noise
int getTerrainHeight(BiomeDefinition *biome, int x, int z);

// Get block at position using configured biome
uint16_t getTerrainBlock(BiomeDefinition *biome, int x, int y, int z, int terrain_height);

// Generate features for a chunk (trees, ores, etc.)
void generateFeatures(BiomeDefinition *biome, int cx, int cy, int cz, uint8_t *chunk_data, int terrain_height[16][16]);

// Check if cave should exist at position
int isCave(BiomeDefinition *biome, int x, int y, int z);

#endif
