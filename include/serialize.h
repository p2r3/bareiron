#ifndef SERIALIZE_H
#define SERIALIZE_H

#include "globals.h"
#include "context.h"

#ifdef SYNC_WORLD_TO_DISK
  int initSerializer (ServerContext *ctx);
  void writeBlockChangesToDisk (ServerContext *ctx, int from, int to);
  void writeChestChangesToDisk (ServerContext *ctx, uint8_t *storage_ptr, uint8_t slot);
  void writePlayerDataToDisk (ServerContext *ctx);
  void writeDataToDiskOnInterval (ServerContext *ctx);
#else
  // Define no-op placeholders for when disk syncing isn't enabled
  #define writeBlockChangesToDisk(ctx, a, b)
  #define writeChestChangesToDisk(ctx, a, b)
  #define writePlayerDataToDisk(ctx)
  #define writeDataToDiskOnInterval(ctx)
  #define initSerializer(ctx) 0
#endif

#endif
