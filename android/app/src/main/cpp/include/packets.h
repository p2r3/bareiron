#ifndef H_PACKETS
#define H_PACKETS

// Serverbound packets
int cs_handshake (int client_fd);
int cs_loginStart (int client_fd, uint8_t *uuid, char *name);
int cs_clientInformation (int client_fd);
int cs_pluginMessage (int client_fd);
int cs_playerAction (int client_fd);
int cs_useItemOn (int client_fd);
int cs_useItem (int client_fd);
int cs_setPlayerPositionAndRotation (int client_fd, double *x, double *y, double *z, float *yaw, float *pitch, uint8_t *on_ground);
int cs_setPlayerPosition (int client_fd, double *x, double *y, double *z, uint8_t *on_ground);
int cs_setPlayerRotation (int client_fd, float *yaw, float *pitch, uint8_t *on_ground);
int cs_setPlayerMovementFlags (int client_fd, uint8_t *on_ground);
int cs_setHeldItem (int client_fd);
int cs_swingArm (int client_fd);
int cs_clickContainer (int client_fd);
int cs_closeContainer (int client_fd);
int cs_clientStatus (int client_fd);
int cs_chat (int client_fd);
int cs_interact (int client_fd);
int cs_playerInput (int client_fd);
int cs_playerCommand (int client_fd);
int cs_playerLoaded (int client_fd);

// Clientbound packets
int sc_statusResponse (int client_fd);
int sc_loginSuccess (int client_fd, uint8_t *uuid, char *name);
int sc_knownPacks (int client_fd);
int sc_sendPluginMessage (int client_fd, const char *channel, const uint8_t *data, size_t data_len);
int sc_finishConfiguration (int client_fd);
int sc_loginPlay (int client_fd);
int sc_synchronizePlayerPosition (int client_fd, double x, double y, double z, float yaw, float pitch);
int sc_setDefaultSpawnPosition (int client_fd, int64_t x, int64_t y, int64_t z);
int sc_startWaitingForChunks (int client_fd);
int sc_playerAbilities (int client_fd, uint8_t flags);
int sc_updateTime (int client_fd, uint64_t ticks);
int sc_setCenterChunk (int client_fd, int x, int y);
int sc_chunkDataAndUpdateLight (int client_fd, int _x, int _z);
int sc_keepAlive (int client_fd);
int sc_setContainerSlot (int client_fd, int window_id, uint16_t slot, uint8_t count, uint16_t item);
int sc_setCursorItem (int client_fd, uint16_t item, uint8_t count);
int sc_setHeldItem (int client_fd, uint8_t slot);
int sc_blockUpdate (int client_fd, int64_t x, int64_t y, int64_t z, uint8_t block);
int sc_openScreen (int client_fd, uint8_t window, const char *title, uint16_t length);
int sc_acknowledgeBlockChange (int client_fd, int sequence);
int sc_playerInfoUpdateAddPlayer (int client_fd, PlayerData player);
int sc_spawnEntity (int client_fd, int id, uint8_t *uuid, int type, double x, double y, double z, uint8_t yaw, uint8_t pitch);
int sc_spawnEntityPlayer (int client_fd, PlayerData player);
int sc_setEntityMetadata (int client_fd, int id, EntityData *metadata, size_t length);
int sc_entityAnimation (int client_fd, int id, uint8_t animation);
int sc_teleportEntity (int client_fd, int id, double x, double y, double z, float yaw, float pitch);
int sc_setHeadRotation (int client_fd, int id, uint8_t yaw);
int sc_updateEntityRotation (int client_fd, int id, uint8_t yaw, uint8_t pitch);
int sc_damageEvent (int client_fd, int id, int type);
int sc_setHealth (int client_fd, uint8_t health, uint8_t food, uint16_t saturation);
int sc_respawn (int client_fd);
int sc_systemChat (int client_fd, char* message, uint16_t len);
int sc_entityEvent (int client_fd, int entity_id, uint8_t status);
int sc_removeEntity (int client_fd, int entity_id);
int sc_pickupItem (int client_fd, int collected, int collector, uint8_t count);
int sc_registries (int client_fd);

#endif
