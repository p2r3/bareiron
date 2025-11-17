#ifndef SERIALIZE_H
#define SERIALIZE_H

#include "globals.h"

#ifdef SYNC_WORLD_TO_DISK
  int initSerializer ();
  void writeBlockChangesToDisk (int from, int to);
  void writeChestChangesToDisk (uint8_t *storage_ptr, uint8_t slot);
  void writePlayerDataToDisk ();
  void writeDataToDiskOnInterval ();
#else
  // Define no-op placeholders for when disk syncing isn't enabled
  #define writeBlockChangesToDisk(a, b)
  #define writeChestChangesToDisk(a, b)
  #define writePlayerDataToDisk()
  #define writeDataToDiskOnInterval()
  #define initSerializer() 0
#endif

#endif
