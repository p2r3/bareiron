
#include "globals.h"
#include "tools.h"
#include "registries.h"
#include "worldgen.h"
#include "procedures.h"
#include "structures.h"

void setBlockIfReplaceable (short x, uint8_t y, short z, uint8_t block) {
  uint8_t target = getBlockAt(x, y, z);
  if (!isReplaceableBlock(target) && target != B_oak_leaves) return;
  makeBlockChange(x, y, z, block);
}

// Places a tree centered on the input coordinates
void placeTreeStructure (short x, uint8_t y, short z) {

  // Get a random number for tree height and leaf edges
  uint32_t r = fast_rand();
  uint8_t height = 4 + (r % 3);

  // Set tree base - replace sapling with log and put dirt below
  makeBlockChange(x, y - 1, z, B_dirt);
  makeBlockChange(x, y, z, B_oak_log);
  // Create tree stump
  for (int i = 1; i < height; i ++) {
    setBlockIfReplaceable(x, y + i, z, B_oak_log);
  }
  // Keep track of leaf corners, determines random number bit shift
  uint8_t t = 2;
  // First (bottom) leaf layer
  for (int i = -2; i <= 2; i ++) {
    for (int j = -2; j <= 2; j ++) {
      setBlockIfReplaceable(x + i, y + height - 3, z + j, B_oak_leaves);
      // Randomly skip some corners, emulating vanilla tree shape
      if ((i == 2 || i == -2) && (j == 2 || j == -2)) {
        t ++;
        if ((r >> t) & 1) continue;
      }
      setBlockIfReplaceable(x + i, y + height - 2, z + j, B_oak_leaves);
    }
  }
  // Second (top) leaf layer
  for (int i = -1; i <= 1; i ++) {
    for (int j = -1; j <= 1; j ++) {
      setBlockIfReplaceable(x + i, y + height - 1, z + j, B_oak_leaves);
      if ((i == 1 || i == -1) && (j == 1 || j == -1)) {
        t ++;
        if ((r >> t) & 1) continue;
      }
      setBlockIfReplaceable(x + i, y + height, z + j, B_oak_leaves);
    }
  }

}

// Places an explosion crater centered on the input coordinates
void placeCraterStructure (short x, uint8_t y, short z, uint8_t radius) {
  int radius_sq = radius * radius;
  
  // Air blocks are placed in a sphere
  // Starting with y to check for height bounds
  for (int dy = -radius; dy <= radius; dy++) {
    if (y + dy < 0 || y + dy > 255) continue;
    int dy_sq = dy * dy;
    for (int dx = -radius; dx <= radius; dx++) {
      int dx_sq = dx * dx;
      for (int dz = -radius; dz <= radius; dz++) {
        int dist_sq = dx_sq + dy_sq + (dz * dz);
        if (dist_sq <= radius_sq) {
          // Don't replace what is already air
          uint8_t current_block = getBlockAt(x + dx, y + dy, z + dz);
          if (current_block != B_air) {
            makeBlockChange(x + dx, y + dy, z + dz, B_air);
          }
        }
      }
    }
  }
}
