#ifndef H_PROCEDURES
#define H_PROCEDURES

#include <unistd.h>
#include "context.h"
#include "globals.h"

// client_states has been moved into ServerContext
void setClientState (ServerContext *ctx, int client_fd, int new_state);
int getClientState (ServerContext *ctx, int client_fd);
int getClientIndex (ServerContext *ctx, int client_fd);

void resetPlayerData (PlayerData *player);
int reservePlayerData (ServerContext *ctx, int client_fd, uint8_t *uuid, char *name);
int getPlayerData (ServerContext *ctx, int client_fd, PlayerData **output);
void handlePlayerDisconnect (ServerContext *ctx, int client_fd);
void handlePlayerJoin (ServerContext *ctx, PlayerData* player);
int givePlayerItem (PlayerData *player, uint16_t item, uint8_t count);
void spawnPlayer (ServerContext *ctx, PlayerData *player);

void broadcastPlayerMetadata (ServerContext *ctx, PlayerData *player);
void broadcastMobMetadata (ServerContext *ctx, int client_fd, int entity_id);

uint8_t serverSlotToClientSlot (int window_id, uint8_t slot);
uint8_t clientSlotToServerSlot (int window_id, uint8_t slot);

uint8_t getBlockChange (ServerContext *ctx, short x, uint8_t y, short z);
uint8_t makeBlockChange (ServerContext *ctx, short x, uint8_t y, short z, uint8_t block);

uint8_t isInstantlyMined (PlayerData *player, uint8_t block);
uint8_t isColumnBlock (uint8_t block);
uint8_t isPassableBlock (uint8_t block);
uint8_t isPassableSpawnBlock (uint8_t block);
uint8_t isReplaceableBlock (uint8_t block);
uint32_t isCompostItem (uint16_t item);
uint8_t getItemStackSize (uint16_t item);

uint16_t getMiningResult (ServerContext *ctx, uint16_t held_item, uint8_t block);
void bumpToolDurability (ServerContext *ctx, PlayerData *player);
void handlePlayerAction (ServerContext *ctx, PlayerData *player, int action, short x, short y, short z);
void handlePlayerUseItem (ServerContext *ctx, PlayerData *player, short x, short y, short z, uint8_t face);

void checkFluidUpdate (ServerContext *ctx, short x, uint8_t y, short z, uint8_t block);

void spawnMob (ServerContext *ctx, uint8_t type, short x, uint8_t y, short z, uint8_t health);
void interactEntity (ServerContext *ctx, int entity_id, int interactor_id);
void hurtEntity (ServerContext *ctx, int entity_id, int attacker_id, uint8_t damage_type, uint8_t damage);
void handleServerTick (ServerContext *ctx, int64_t time_since_last_tick);

void broadcastChestUpdate (ServerContext *ctx, int origin_fd, uint8_t *storage_ptr, uint16_t item, uint8_t count, uint8_t slot);

ssize_t writeEntityData (int client_fd, EntityData *data);

int sizeEntityData (EntityData *data);
int sizeEntityMetadata (EntityData *metadata, size_t length);

// Completes the transition into PLAY state and performs the full
// join/spawn/broadcast sequence for a player after configuration.
void completePlayerSpawnSequence(ServerContext *ctx, int client_fd);

#endif
