#ifndef H_PROCEDURES
#define H_PROCEDURES

#include <unistd.h>

#include "globals.h"

extern int client_states[MAX_PLAYERS * 2];

void setClientState (int client_fd, int new_state);
int getClientState (int client_fd);
int getClientIndex (int client_fd);

void resetPlayerData (PlayerData *player);
int reservePlayerData (int client_fd, uint8_t *uuid, char* name);
int getPlayerData (int client_fd, PlayerData **output);
void handlePlayerDisconnect (int client_fd);
void handlePlayerJoin (PlayerData* player);
void disconnectClient (int *client_fd, int cause);
int givePlayerItem (PlayerData *player, uint16_t item, uint8_t count);
void spawnPlayer (PlayerData *player);

void broadcastPlayerMetadata (PlayerData *player);

uint8_t serverSlotToClientSlot (int window_id, uint8_t slot);
uint8_t clientSlotToServerSlot (int window_id, uint8_t slot);

uint8_t getBlockChange (short x, uint8_t y, short z);
uint8_t makeBlockChange (short x, uint8_t y, short z, uint8_t block);

uint8_t isInstantlyMined (PlayerData *player, uint8_t block);
uint8_t isColumnBlock (uint8_t block);
uint8_t isPassableBlock (uint8_t block);
uint8_t isPassableSpawnBlock (uint8_t block);
uint8_t isReplaceableBlock (uint8_t block);
uint32_t isCompostItem (uint16_t item);
uint8_t getItemStackSize (uint16_t item);

uint16_t getMiningResult (uint16_t held_item, uint8_t block);
void bumpToolDurability (PlayerData *player);
void handlePlayerAction (PlayerData *player, int action, short x, short y, short z);
void handlePlayerUseItem (PlayerData *player, short x, short y, short z, uint8_t face);

void checkFluidUpdate (short x, uint8_t y, short z, uint8_t block);

void spawnMob (uint8_t type, short x, uint8_t y, short z, uint8_t health);
void hurtEntity (int entity_id, int attacker_id, uint8_t damage_type, uint8_t damage);
void handleServerTick (int64_t time_since_last_tick);

void broadcastChestUpdate (int origin_fd, uint8_t *storage_ptr, uint16_t item, uint8_t count, uint8_t slot);

ssize_t writeEntityData (int client_fd, EntityData *data);

int sizeEntityData (EntityData *data);
int sizeEntityMetadata (EntityData *metadata, size_t length);

#endif
