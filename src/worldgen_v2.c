#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "worldgen_v2.h"
#include "biome_config.h"
#include "noise.h"
#include "globals.h"
#include "procedures.h"

#ifdef BLOCKS_COUNT
  #include "registries.h"
#else
  // Fallback block IDs when registries aren't generated
  #define B_air 0
  #define B_stone 1
  #define B_grass_block 9
  #define B_dirt 10
  #define B_bedrock 7
  #define B_water 34
#endif

int getTerrainHeight(BiomeDefinition *biome, int x, int z) {
  // Get height from configured noise layer
  float noise_val = get_noise_value(biome->height_noise, (float)x, (float)z);

  // Normalize to [0, 1]
  noise_val = noise_normalize(noise_val);

  // Apply biome's height scaling
  int height = biome->base_height + (int)(noise_val * biome->height_scale);

  // Add detail noise for micro-variations
  float detail = get_noise_value("detail", (float)x, (float)z);
  height += (int)(detail * 3.0f);

  // Clamp to reasonable range
  if (height < 0) height = 0;
  if (height > 255) height = 255;

  return height;
}

int isCave(BiomeDefinition *biome, int x, int y, int z) {
  if (!biome->caves_enabled) return 0;

  // Use 3D noise for caves (approximate with 2D + Y influence)
  float cave_noise = get_noise_value("cave_noise", (float)x + y * 0.1f, (float)z + y * 0.1f);

  return (fabsf(cave_noise) < biome->cave_threshold - 0.5f);
}

uint16_t getTerrainBlock(BiomeDefinition *biome, int x, int y, int z, int terrain_height) {
  // Air above terrain
  if (y > terrain_height) {
    // Fill with water below sea level
    if (y < 64) return B_water;
    return B_air;
  }

  // Check for caves
  if (y < terrain_height - 4 && isCave(biome, x, y, z)) {
    return B_air;
  }

  // Apply surface layers
  int depth_from_surface = terrain_height - y;
  int accumulated_depth = 0;

  for (int i = 0; i < biome->surface_layer_count; i++) {
    SurfaceLayer *layer = &biome->surface_layers[i];

    if (layer->depth < 0) {
      // Infinite depth - fill to bedrock
      return layer->block_id;
    }

    accumulated_depth += layer->depth;
    if (depth_from_surface < accumulated_depth) {
      return layer->block_id;
    }
  }

  // Fallback to stone
  return B_stone;
}

void generateFeatures(BiomeDefinition *biome, int cx, int cy, int cz, uint8_t *chunk_data, int terrain_height[16][16]) {
  // Generate features like trees, ores, scatter blocks

  for (int i = 0; i < biome->feature_count; i++) {
    BiomeFeature *feature = &biome->features[i];

    switch (feature->type) {
      case FEATURE_TREE: {
        // Try to place trees
        for (int attempts = 0; attempts < 3; attempts++) {
          // Use chunk hash for deterministic but random-looking placement
          uint32_t hash = splitmix64((uint64_t)cx + ((uint64_t)cz << 16) + ((uint64_t)attempts << 32));
          float chance = (float)(hash % 10000) / 10000.0f;

          if (chance > feature->chance) continue;

          int tree_x = cx + (hash % 16);
          int tree_z = cz + ((hash >> 8) % 16);
          int rx = tree_x - cx;
          int rz = tree_z - cz;

          if (rx < 0 || rx >= 16 || rz < 0 || rz >= 16) continue;

          int ground_height = terrain_height[rx][rz];
          int tree_y = ground_height + 1;

          // Check if tree is in this chunk section
          if (tree_y < cy || tree_y >= cy + 16) continue;

          int tree_height = feature->min_height + (hash % (feature->max_height - feature->min_height + 1));

          // Place tree trunk
          for (int ty = 0; ty < tree_height && tree_y + ty < cy + 16; ty++) {
            int idx = rx + rz * 16 + (tree_y + ty - cy) * 256;
            if (idx >= 0 && idx < 4096) {
              chunk_data[idx] = feature->block_id;
            }
          }

          // Place leaves (simple blob)
          for (int lx = -2; lx <= 2; lx++) {
            for (int lz = -2; lz <= 2; lz++) {
              for (int ly = tree_height - 3; ly < tree_height + 1; ly++) {
                if (abs(lx) + abs(lz) > 3) continue;

                int leaf_x = tree_x + lx;
                int leaf_z = tree_z + lz;
                int leaf_y = tree_y + ly;

                if (leaf_y < cy || leaf_y >= cy + 16) continue;
                if (leaf_x < cx || leaf_x >= cx + 16) continue;
                if (leaf_z < cz || leaf_z >= cz + 16) continue;

                int lrx = leaf_x - cx;
                int lrz = leaf_z - cz;
                int idx = lrx + lrz * 16 + (leaf_y - cy) * 256;

                if (idx >= 0 && idx < 4096 && chunk_data[idx] == B_air) {
                  chunk_data[idx] = feature->leaves_id;
                }
              }
            }
          }
        }
        break;
      }

      case FEATURE_SCATTER: {
        // Scatter blocks on surface
        for (int rx = 0; rx < 16; rx++) {
          for (int rz = 0; rz < 16; rz++) {
            int x = cx + rx;
            int z = cz + rz;
            int height = terrain_height[rx][rz];

            if (height + 1 < cy || height + 1 >= cy + 16) continue;

            uint32_t hash = splitmix64((uint64_t)x + ((uint64_t)z << 16) + biome->id);
            float chance = (float)(hash % 10000) / 10000.0f;

            if (chance < feature->chance) {
              int idx = rx + rz * 16 + (height + 1 - cy) * 256;
              if (idx >= 0 && idx < 4096) {
                chunk_data[idx] = feature->block_id;
              }
            }
          }
        }
        break;
      }

      case FEATURE_ORE: {
        // Generate ores underground
        for (int rx = 0; rx < 16; rx++) {
          for (int rz = 0; rz < 16; rz++) {
            for (int ry = 0; ry < 16; ry++) {
              int y = cy + ry;

              if (y < feature->min_y || y > feature->max_y) continue;

              int x = cx + rx;
              int z = cz + rz;

              uint32_t hash = splitmix64((uint64_t)x + ((uint64_t)y << 16) + ((uint64_t)z << 32));
              float chance = (float)(hash % 10000) / 10000.0f;

              if (chance < feature->chance / 100.0f) {  // Ores are rarer
                int idx = rx + rz * 16 + ry * 256;
                if (idx >= 0 && idx < 4096 && chunk_data[idx] == B_stone) {
                  chunk_data[idx] = feature->block_id;
                }
              }
            }
          }
        }
        break;
      }

      default:
        break;
    }
  }
}

uint8_t buildChunkSectionV2(int cx, int cy, int cz, uint8_t *output) {
  // Check if world config is loaded
  if (world_config.biome_count == 0) {
    fprintf(stderr, "Error: World config not loaded\n");
    memset(output, 0, 4096);  // Fill with air
    return 0;
  }

  // Precompute terrain heights for this chunk section
  int terrain_height[16][16];
  BiomeDefinition *chunk_biome = NULL;

  for (int rx = 0; rx < 16; rx++) {
    for (int rz = 0; rz < 16; rz++) {
      int x = cx + rx;
      int z = cz + rz;

      // Get biome at this position (we'll use the corner biome for the whole chunk)
      if (rx == 0 && rz == 0) {
        chunk_biome = get_biome_at((float)x, (float)z);
      }

      // Use the chunk's primary biome for all positions (for simplicity)
      // In a more advanced version, you'd blend biomes
      terrain_height[rx][rz] = getTerrainHeight(chunk_biome, x, z);
    }
  }

  // Generate base terrain
  for (int y = 0; y < 16; y++) {
    for (int z = 0; z < 16; z++) {
      for (int x = 0; x < 16; x++) {
        int world_y = cy + y;
        int world_x = cx + x;
        int world_z = cz + z;

        // Bedrock at bottom
        if (world_y < 0) {
          output[x + z * 16 + y * 256] = B_bedrock;
          continue;
        }

        uint16_t block = getTerrainBlock(chunk_biome, world_x, world_y, world_z, terrain_height[x][z]);
        output[x + z * 16 + y * 256] = (uint8_t)block;
      }
    }
  }

  // Generate features
  generateFeatures(chunk_biome, cx, cy, cz, output, terrain_height);

  // Apply block changes from player modifications
  for (int i = 0; i < block_changes_count; i++) {
    if (block_changes[i].block == 0xFF) continue;

    if (block_changes[i].x >= cx && block_changes[i].x < cx + 16 &&
        block_changes[i].y >= cy && block_changes[i].y < cy + 16 &&
        block_changes[i].z >= cz && block_changes[i].z < cz + 16) {

      int dx = block_changes[i].x - cx;
      int dy = block_changes[i].y - cy;
      int dz = block_changes[i].z - cz;

      output[dx + dz * 16 + dy * 256] = block_changes[i].block;
    }
  }

  return chunk_biome ? chunk_biome->id : 0;
}
