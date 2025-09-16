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

// Pass ServerContext to eliminate globals (internals may ignore ctx temporarily)
#include "context.h"
uint32_t getChunkHash (ServerContext *ctx, short x, short z);
uint8_t getChunkBiome (ServerContext *ctx, short x, short z);
uint8_t getHeightAtFromHash (ServerContext *ctx, int rx, int rz, int _x, int _z, uint32_t chunk_hash, uint8_t biome);
uint8_t getHeightAt (ServerContext *ctx, int x, int z);
uint8_t getTerrainAt (ServerContext *ctx, int x, int y, int z, ChunkAnchor anchor);
uint8_t getBlockAt (ServerContext *ctx, int x, int y, int z);

extern uint8_t chunk_section[4096];
uint8_t buildChunkSection (ServerContext *ctx, int cx, int cy, int cz);

#endif
