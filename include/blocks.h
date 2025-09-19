#ifndef H_BLOCKS
#define H_BLOCKS

#include <unistd.h>
#include "globals.h"

typedef struct {
  short x;
  short z;
  uint8_t y;
  uint8_t block;
} BlockChange;

typedef struct {
  int ri; // run index
  int li; // run-local block index
} BlockRef;

typedef struct {
  BlockChange runs[MAX_BLOCK_CHANGES];
  short lens[MAX_BLOCK_CHANGES]; // 2b x-mode, 2b y-mode, 2b z-mode, 10b run length
} BlockBuf;

BlockRef nextBlockChange (BlockRef curr);
BlockChange derefBlockChange (BlockRef ref);

uint8_t getBlockChange (short x, uint8_t y, short z);
uint8_t makeBlockChange (short x, uint8_t y, short z, uint8_t block);

extern BlockBuf block_changes;
extern int block_changes_count;

#endif
