#ifndef CRAFTING_H
#define CRAFTING_H

#include "globals.h"

void getCraftingOutput (ServerContext *ctx, PlayerData *player, uint8_t *count, uint16_t *item);
void getSmeltingOutput (ServerContext *ctx, PlayerData *player);

#endif
