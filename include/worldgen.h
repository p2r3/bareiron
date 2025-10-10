#ifndef H_WORLDGEN
#define H_WORLDGEN

#include <stdint.h>

typedef struct {
  short x;
  short z;
  uint32_t hash;
  uint8_t biome;
} ChunkAnchor;

typedef struct {
  short x;
  uint8_t y;
  short z;
  uint8_t variant;
} ChunkFeature;

uint32_t getChunkHash (short x, short z);
uint8_t getChunkBiome (short x, short z);
uint8_t getHeightAtFromHash (int rx, int rz, int _x, int _z, uint32_t chunk_hash, uint8_t biome);
uint8_t getHeightAt (int x, int z);
uint8_t getTerrainAt (int x, int y, int z, ChunkAnchor anchor);
uint8_t getBlockAt (int x, int y, int z);

extern uint8_t chunk_section[4096];
uint8_t buildChunkSection (int cx, int cy, int cz);

#endif
