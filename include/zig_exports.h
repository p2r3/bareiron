#ifndef H_ZIG_EXPORTS
#define H_ZIG_EXPORTS

#include "context.h" // We need PlayerData and ServerContext types

// Declare the functions that are exported from Zig.
// 'extern' tells the C compiler "this function exists, but its
// actual code is somewhere else (in the Zig object file)."
extern void getCraftingOutputC(ServerContext *ctx, PlayerData *player, uint8_t *count, uint16_t *item);
extern void getSmeltingOutput(ServerContext *ctx, PlayerData *player);

#endif