#include "globals.h"
#include "packets.h"
#include "worldgen.h"
#include "blocks.h"

#include <stdint.h>
#include <stdio.h>

BlockBuf block_changes;
int block_changes_count;

BlockRef nextBlockChange (BlockRef curr) {
  if (curr.ri >= block_changes_count) return (BlockRef){-1, -1};

  short rlen = block_changes.lens[curr.ri];
  rlen &= 0x3FF; // mask out run flags from length

  curr.li ++;
  if (curr.li == rlen) {
    // end of run, load the next one
    curr.ri ++;
    curr.li = 0;

    if (curr.ri == block_changes_count) return (BlockRef){-1, -1};
    return curr;
  }

  // return next block in run
  return curr;
}

static inline int modeToOffset (uint8_t mode) {
  switch (mode & 3) {
  case 0 /* 0b00 */: return 0;
  case 1 /* 0b01 */: // reserved
  case 2 /* 0b10 */: return 1;
  case 3 /* 0b11 */: return -1;
  }
}

static inline uint8_t offsetToMode (int offset) {
  switch (offset) {
  case 0: return 0;
  case 1: return 2;
  case -1: return 3;
  }
}

static inline int isInRunBounds (uint8_t mode, short base, int len, short val) {
  switch (mode) {
  case 0: return base == val;
  case 2: return base <= val && base + len > val;
  case 3: return base >= val && base - len < val;
  }
}

BlockChange derefBlockChange (BlockRef ref) {
  if (ref.ri == -1) return (BlockChange){0, 0, 0, 0xFF};

  // load len and axis modes
  short rlen = block_changes.lens[ref.ri];
  uint8_t x_mode = (rlen >> 14) & 3;
  uint8_t y_mode = (rlen >> 12) & 3;
  uint8_t z_mode = (rlen >> 10) & 3;
  rlen &= 0x3FF;

  // load base change and apply axis offsets
  BlockChange change = block_changes.runs[ref.ri];
  change.x += modeToOffset(x_mode) * ref.li;
  change.y += modeToOffset(y_mode) * ref.li;
  change.z += modeToOffset(z_mode) * ref.li;

  return change;
}

uint8_t getBlockChange(short x, uint8_t y, short z) {
  for (BlockRef ref = {0}; ref.ri != -1; ref = nextBlockChange(ref)) {
    BlockChange change = derefBlockChange(ref);
    if (change.block == 0xFF) continue;
    if (change.x == x &&
        change.y == y &&
        change.z == z
    ) return change.block;
  }
  return 0xFF;
}

// Handle running out of memory for new block changes
void failBlockChange (short x, uint8_t y, short z, uint8_t block) {

  // Get previous block at this location
  uint8_t before = getBlockAt(x, y, z);

  // Broadcast a new update to all players
  for (int i = 0; i < MAX_PLAYERS; i ++) {
    if (player_data[i].client_fd == -1) continue;
    if (player_data[i].flags & 0x20) continue;
    // Reset the block they tried to change
    sc_blockUpdate(player_data[i].client_fd, x, y, z, before);
    // Broadcast a chat message warning about the limit
    sc_systemChat(player_data[i].client_fd, "Block changes limit exceeded. Restore original terrain to continue.", 67);
  }

}

uint8_t makeBlockChange (short x, uint8_t y, short z, uint8_t block) {

  // Transmit block update to all in-game clients
  for (int i = 0; i < MAX_PLAYERS; i ++) {
    if (player_data[i].client_fd == -1) continue;
    if (player_data[i].flags & 0x20) continue;
    sc_blockUpdate(player_data[i].client_fd, x, y, z, block);
  }

  // Calculate terrain at these coordinates and compare it to the input block.
  // Since block changes get overlayed on top of terrain, we don't want to
  // store blocks that don't differ from the base terrain.
  ChunkAnchor anchor = {
    x / CHUNK_SIZE,
    z / CHUNK_SIZE,
  };
  if (x % CHUNK_SIZE < 0) anchor.x --;
  if (z % CHUNK_SIZE < 0) anchor.z --;
  anchor.hash = getChunkHash(anchor.x, anchor.z);
  anchor.biome = getChunkBiome(anchor.x, anchor.z);

  uint8_t is_base_block = block == getTerrainAt(x, y, z, anchor);

  // Prioritize inserting changes into already existing runs
  // to compress changes as much as possible
  for (int i = 0; i < block_changes_count; i ++) {
    BlockChange change = block_changes.runs[i];

    short rlen = block_changes.lens[i];
    uint8_t x_mode = (rlen >> 14) & 3;
    uint8_t y_mode = (rlen >> 12) & 3;
    uint8_t z_mode = (rlen >> 10) & 3;
    rlen &= 0x3FF;

    // check if the coords are covered by the run and remove the block from the run if found
    if (isInRunBounds(x_mode, change.x, rlen, x) &&
        isInRunBounds(y_mode, change.y, rlen, y) &&
        isInRunBounds(z_mode, change.z, rlen, z)
      ) {
      // if (rlen == 1) continue; // should never happen

      // figure out the local-index of the block
      int li;
      if (x_mode != 0) li = (x - change.x) / modeToOffset(x_mode);
      else if (y_mode != 0) li = (y - change.y) / modeToOffset(y_mode);
      else if (z_mode != 0) li = (z - change.z) / modeToOffset(z_mode);

      if (li == 0) {
        if (rlen == 1) {
          // no blocks in run, fill gap with a different run
          block_changes_count --;
          if (block_changes_count == 0) break;

          block_changes.runs[i] = block_changes.runs[block_changes_count];
          block_changes.lens[i] = block_changes.lens[block_changes_count];

          #ifdef DEV_LOG_BLOCK_STORAGE_STATS
            printf("Block storage stats\n");
            printf("  Block run usage: %d runs - %dB\n\n", block_changes_count, block_changes_count * (sizeof(BlockChange) + sizeof(int)));
          #endif
          i --; // rescan the currently moved run
          continue;
        }

        // push run start by one to remove the block
        change.x += modeToOffset(x_mode);
        change.y += modeToOffset(y_mode);
        change.z += modeToOffset(z_mode);
        block_changes.runs[i] = change;
        block_changes.lens[i] --;

        continue;
      } else if (li == rlen - 1) {
        if (rlen == 1) {
          // no blocks in run, fill gap with a different run
          block_changes_count --;
          if (block_changes_count == 0) break;

          block_changes.runs[i] = block_changes.runs[block_changes_count];
          block_changes.lens[i] = block_changes.lens[block_changes_count];

          #ifdef DEV_LOG_BLOCK_STORAGE_STATS
            printf("Block storage stats\n");
            printf("  Block run usage: %d runs - %dB\n\n", block_changes_count, block_changes_count * (sizeof(BlockChange) + sizeof(int)));
          #endif
          i --; // rescan the currently moved run
          continue;
        }

        // shorten run by one end by one
        block_changes.lens[i] --;
        continue;
      } else {
        // block is in the middle of the run, split
        // the runs into two

        if (block_changes_count == MAX_BLOCK_CHANGES) {
          failBlockChange(x, y, z, block);
          return 1;
        }

        // create the first half by just shortening the current run
        short first_len = li;
        first_len |= (x_mode << 14);
        first_len |= (y_mode << 12);
        first_len |= (z_mode << 10);
        block_changes.lens[i] = first_len;

        // push the second half as a new run to the end of the buffer
        BlockChange second_change = change;
        second_change.x += modeToOffset(x_mode) * (li + 1);
        second_change.y += modeToOffset(y_mode) * (li + 1);
        second_change.z += modeToOffset(z_mode) * (li + 1);

        short second_len = rlen - li - 1;
        second_len |= (x_mode << 14);
        second_len |= (y_mode << 12);
        second_len |= (z_mode << 10);

        block_changes.runs[block_changes_count] = second_change;
        block_changes.lens[block_changes_count] = second_len;
        block_changes_count ++;
      }
    }

    // try to add a block only if the run matches
    if (change.block != block || is_base_block) continue;

    if (rlen == 1) {
      // check if starting a new run is possible
      int x_off = x - change.x;
      int y_off = y - change.y;
      int z_off = z - change.z;

      if ((x_off <= 1 && x_off >= -1) &&
          (y_off <= 1 && y_off >= -1) &&
          (z_off <= 1 && z_off >= -1)
        ) {
        rlen = 2;
        rlen |= (offsetToMode(x_off) << 14);
        rlen |= (offsetToMode(y_off) << 12);
        rlen |= (offsetToMode(z_off) << 10);

        // insert into run
        block_changes.lens[i] = rlen;
        return 0;
      }
    } else {
      // check if change can be appended to run
      if (change.x + modeToOffset(x_mode) * rlen == x &&
          change.y + modeToOffset(y_mode) * rlen == y &&
          change.z + modeToOffset(z_mode) * rlen == z
        ) {
        if (rlen == 0x3FF) continue; // do not overflow the len int
        block_changes.lens[i] ++;    // increment the run length to include the new change
        return 0;
      }
    }
  }

  // Don't create a new entry if it contains the base terrain block
  if (is_base_block) return 0;

  // Handle running out of memory for new block changes
  if (block_changes_count == MAX_BLOCK_CHANGES) {
    failBlockChange(x, y, z, block);
    return 1;
  }

  // Fall back to storing the change at the end of the buffer
  block_changes.runs[block_changes_count] = (BlockChange){
      x, z, y,
      block,
  };
  block_changes.lens[block_changes_count] = 1;
  block_changes_count ++;

  #ifdef DEV_LOG_BLOCK_STORAGE_STATS
    printf("Block storage stats\n");
    printf("  Block run usage: %d runs - %dB\n\n", block_changes_count, block_changes_count * (sizeof(BlockChange) + sizeof(int)));
  #endif

  // Write change to disk (if applicable)
  // FIXME: sync writes

  return 0;

}
