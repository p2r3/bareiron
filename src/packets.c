#include <stdio.h>
#include <string.h>

#ifdef ESP_PLATFORM
  #include "lwip/sockets.h"
  #include "lwip/netdb.h"
  #include "esp_task_wdt.h"
#else
  #ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
  #else
    #include <arpa/inet.h>
  #endif
  #include <unistd.h>
#endif

#include "globals.h"
#include "tools.h"
#include "varnum.h"
#include "registries.h"
#include "worldgen.h"
#include "crafting.h"
#include "procedures.h"
#include "packets.h"

// S->C Status Response (server list ping)
int sc_statusResponse (int client_fd) {

  char header[] = "{"
    "\"version\":{\"name\":\"1.21.8\",\"protocol\":772},"
    "\"description\":{\"text\":\"";
  char footer[] = "\"}}";

  uint16_t string_len = sizeof(header) + sizeof(footer) + motd_len - 2;

  writeVarInt(client_fd, 1 + string_len + sizeVarInt(string_len));
  writeByte(client_fd, 0x00);

  writeVarInt(client_fd, string_len);
  send_all(client_fd, header, sizeof(header) - 1);
  send_all(client_fd, motd, motd_len);
  send_all(client_fd, footer, sizeof(footer) - 1);

  return 0;
}

// C->S Handshake
int cs_handshake (int client_fd) {
  printf("Received Handshake:\n");

  printf("  Protocol version: %d\n", (int)readVarInt(client_fd));
  readString(client_fd);
  if (recv_count == -1) return 1;
  printf("  Server address: %s\n", recv_buffer);
  printf("  Server port: %u\n", readUint16(client_fd));
  int intent = readVarInt(client_fd);
  if (intent == VARNUM_ERROR) return 1;
  printf("  Intent: %d\n\n", intent);
  setClientState(client_fd, intent);

  return 0;
}

// C->S Login Start
int cs_loginStart (int client_fd, uint8_t *uuid, char *name) {
  printf("Received Login Start:\n");

  readString(client_fd);
  if (recv_count == -1) return 1;
  strncpy(name, (char *)recv_buffer, 16 - 1);
  name[16 - 1] = '\0';
  printf("  Player name: %s\n", name);
  recv_count = recv_all(client_fd, recv_buffer, 16, false);
  if (recv_count == -1) return 1;
  memcpy(uuid, recv_buffer, 16);
  printf("  Player UUID: ");
  for (int i = 0; i < 16; i ++) printf("%x", uuid[i]);
  printf("\n\n");

  return 0;
}

// S->C Login Success
int sc_loginSuccess (int client_fd, uint8_t *uuid, char *name) {
  printf("Sending Login Success...\n\n");

  uint8_t name_length = strlen(name);
  writeVarInt(client_fd, 1 + 16 + sizeVarInt(name_length) + name_length + 1);
  writeVarInt(client_fd, 0x02);
  send_all(client_fd, uuid, 16);
  writeVarInt(client_fd, name_length);
  send_all(client_fd, name, name_length);
  writeVarInt(client_fd, 0);

  return 0;
}

int cs_clientInformation (int client_fd) {
  int tmp;
  printf("Received Client Information:\n");
  readString(client_fd);
  if (recv_count == -1) return 1;
  printf("  Locale: %s\n", recv_buffer);
  tmp = readByte(client_fd);
  if (recv_count == -1) return 1;
  printf("  View distance: %d\n", tmp);
  tmp = readVarInt(client_fd);
  if (recv_count == -1) return 1;
  printf("  Chat mode: %d\n", tmp);
  tmp = readByte(client_fd);
  if (recv_count == -1) return 1;
  if (tmp) printf("  Chat colors: on\n");
  else printf("  Chat colors: off\n");
  tmp = readByte(client_fd);
  if (recv_count == -1) return 1;
  printf("  Skin parts: %d\n", tmp);
  tmp = readVarInt(client_fd);
  if (recv_count == -1) return 1;
  if (tmp) printf("  Main hand: right\n");
  else printf("  Main hand: left\n");
  tmp = readByte(client_fd);
  if (recv_count == -1) return 1;
  if (tmp) printf("  Text filtering: on\n");
  else printf("  Text filtering: off\n");
  tmp = readByte(client_fd);
  if (recv_count == -1) return 1;
  if (tmp) printf("  Allow listing: on\n");
  else printf("  Allow listing: off\n");
  tmp = readVarInt(client_fd);
  if (recv_count == -1) return 1;
  printf("  Particles: %d\n\n", tmp);
  return 0;
}

// S->C Clientbound Known Packs
int sc_knownPacks (int client_fd) {
  printf("Sending Server's Known Packs\n\n");
  char known_packs[] = {
    0x0e, 0x01, 0x09, 0x6d, 0x69, 0x6e,
    0x65, 0x63, 0x72, 0x61, 0x66, 0x74, 0x04, 0x63,
    0x6f, 0x72, 0x65, 0x06, 0x31, 0x2e, 0x32, 0x31,
    0x2e, 0x38
  };
  writeVarInt(client_fd, 24);
  send_all(client_fd, &known_packs, 24);
  return 0;
}

// C->S Serverbound Plugin Message
int cs_pluginMessage (int client_fd) {
  printf("Received Plugin Message:\n");
  readString(client_fd);
  if (recv_count == -1) return 1;
  printf("  Channel: \"%s\"\n", recv_buffer);
  if (strcmp((char *)recv_buffer, "minecraft:brand") == 0) {
    readString(client_fd);
    if (recv_count == -1) return 1;
    printf("  Brand: \"%s\"\n", recv_buffer);
  }
  printf("\n");
  return 0;
}

// S->C Clientbound Plugin Message
int sc_sendPluginMessage (int client_fd, const char *channel, const uint8_t *data, size_t data_len) {
  printf("Sending Plugin Message\n\n");
  int channel_len = (int)strlen(channel);

  writeVarInt(client_fd, 1 + sizeVarInt(channel_len) + channel_len + sizeVarInt(data_len) + data_len);
  writeByte(client_fd, 0x01);

  writeVarInt(client_fd, channel_len);
  send_all(client_fd, channel, channel_len);

  writeVarInt(client_fd, data_len);
  send_all(client_fd, data, data_len);

  return 0;
}

// S->C Finish Configuration
int sc_finishConfiguration (int client_fd) {
  writeVarInt(client_fd, 1);
  writeVarInt(client_fd, 0x03);
  return 0;
}

// S->C Login (play)
int sc_loginPlay (int client_fd) {

  writeVarInt(client_fd, 47 + sizeVarInt(MAX_PLAYERS) + sizeVarInt(VIEW_DISTANCE) * 2);
  writeByte(client_fd, 0x2B);
  // entity id
  writeUint32(client_fd, client_fd);
  // hardcore
  writeByte(client_fd, false);
  // dimensions
  writeVarInt(client_fd, 1);
  writeVarInt(client_fd, 9);
  const char *dimension = "overworld";
  send_all(client_fd, dimension, 9);
  // maxplayers
  writeVarInt(client_fd, MAX_PLAYERS);
  // view distance
  writeVarInt(client_fd, VIEW_DISTANCE);
  // sim distance
  writeVarInt(client_fd, VIEW_DISTANCE);
  // reduced debug info
  writeByte(client_fd, 0);
  // respawn screen
  writeByte(client_fd, true);
  // limited crafting
  writeByte(client_fd, false);
  // dimension id
  writeVarInt(client_fd, 0);
  // dimension name
  writeVarInt(client_fd, 9);
  send_all(client_fd, dimension, 9);
  // hashed seed
  writeUint64(client_fd, 0x0123456789ABCDEF);
  // gamemode
  writeByte(client_fd, GAMEMODE);
  // previous gamemode
  writeByte(client_fd, 0xFF);
  // is debug
  writeByte(client_fd, 0);
  // is flat
  writeByte(client_fd, 0);
  // has death location
  writeByte(client_fd, 0);
  // portal cooldown
  writeVarInt(client_fd, 0);
  // sea level
  writeVarInt(client_fd, 63);
  // secure chat
  writeByte(client_fd, 0);

  return 0;

}

// S->C Synchronize Player Position
int sc_synchronizePlayerPosition (int client_fd, double x, double y, double z, float yaw, float pitch) {

  writeVarInt(client_fd, 61 + sizeVarInt(-1));
  writeByte(client_fd, 0x41);

  // Teleport ID
  writeVarInt(client_fd, -1);

  // Position
  writeDouble(client_fd, x);
  writeDouble(client_fd, y);
  writeDouble(client_fd, z);

  // Velocity
  writeDouble(client_fd, 0);
  writeDouble(client_fd, 0);
  writeDouble(client_fd, 0);

  // Angles (Yaw/Pitch)
  writeFloat(client_fd, yaw);
  writeFloat(client_fd, pitch);

  // Flags
  writeUint32(client_fd, 0);

  return 0;

}

// S->C Set Default Spawn Position
int sc_setDefaultSpawnPosition (int client_fd, int64_t x, int64_t y, int64_t z) {

  writeVarInt(client_fd, sizeVarInt(0x5A) + 12);
  writeVarInt(client_fd, 0x5A);

  writeUint64(client_fd, ((x & 0x3FFFFFF) << 38) | ((z & 0x3FFFFFF) << 12) | (y & 0xFFF));
  writeFloat(client_fd, 0);

  return 0;
}

// S->C Player Abilities (clientbound)
int sc_playerAbilities (int client_fd, uint8_t flags) {

  writeVarInt(client_fd, 10);
  writeByte(client_fd, 0x39);

  writeByte(client_fd, flags);
  writeFloat(client_fd, 0.05f);
  writeFloat(client_fd, 0.1f);

  return 0;
}

// S->C Update Time
int sc_updateTime (int client_fd, uint64_t ticks) {

  writeVarInt(client_fd, 18);
  writeVarInt(client_fd, 0x6A);

  uint64_t world_age = get_program_time() / 50000;
  writeUint64(client_fd, world_age);
  writeUint64(client_fd, ticks);
  writeByte(client_fd, true);

  return 0;
}

// S->C Game Event 13 (Start waiting for level chunks)
int sc_startWaitingForChunks (int client_fd) {
  writeVarInt(client_fd, 6);
  writeByte(client_fd, 0x22);
  writeByte(client_fd, 13);
  writeUint32(client_fd, 0);
  return 0;
}

// S->C Set Center Chunk
int sc_setCenterChunk (int client_fd, int x, int y) {
  writeVarInt(client_fd, 1 + sizeVarInt(x) + sizeVarInt(y));
  writeByte(client_fd, 0x57);
  writeVarInt(client_fd, x);
  writeVarInt(client_fd, y);
  return 0;
}

// S->C Chunk Data and Update Light
int sc_chunkDataAndUpdateLight (int client_fd, int _x, int _z) {

  const int chunk_data_size = (4101 + sizeVarInt(256) + sizeof(network_block_palette)) * 20 + 6 * 12;
  const int light_data_size = 14 + (sizeVarInt(2048) + 2048) * 26;

  writeVarInt(client_fd, 11 + sizeVarInt(chunk_data_size) + chunk_data_size + light_data_size);
  writeByte(client_fd, 0x27);

  writeUint32(client_fd, _x);
  writeUint32(client_fd, _z);

  writeVarInt(client_fd, 0); // omit heightmaps

  writeVarInt(client_fd, chunk_data_size);

  int x = _x * 16, z = _z * 16, y;

  // send 4 chunk sections (up to Y=0) with no blocks
  for (int i = 0; i < 4; i ++) {
    writeUint16(client_fd, 4096); // block count
    writeByte(client_fd, 0); // block bits
    writeVarInt(client_fd, 85); // block palette (bedrock)
    writeByte(client_fd, 0); // biome bits
    writeByte(client_fd, 0); // biome palette
  }
  // yield to idle task
  task_yield();

  // send chunk sections
  for (int i = 0; i < 20; i ++) {
    y = i * 16;
    writeUint16(client_fd, 4096); // block count
    writeByte(client_fd, 8); // bits per entry
    writeVarInt(client_fd, 256); // block palette length
    // block palette as varint buffer
    send_all(client_fd, network_block_palette, sizeof(network_block_palette));
    // chunk section buffer
    uint8_t biome = buildChunkSection(x, y, z);
    send_all(client_fd, chunk_section, 4096);
    // biome data
    writeByte(client_fd, 0); // bits per entry
    writeByte(client_fd, biome); // biome palette
    // yield to idle task
    task_yield();
  }

  // send 8 chunk sections (up to Y=192) with no blocks
  for (int i = 0; i < 8; i ++) {
    writeUint16(client_fd, 4096); // block count
    writeByte(client_fd, 0); // block bits
    writeVarInt(client_fd, 0); // block palette (air)
    writeByte(client_fd, 0); // biome bits
    writeByte(client_fd, 0); // biome palette
  }
  // yield to idle task
  task_yield();

  writeVarInt(client_fd, 0); // omit block entities

  // light data
  writeVarInt(client_fd, 1);
  writeUint64(client_fd, 0b11111111111111111111111111);
  writeVarInt(client_fd, 0);
  writeVarInt(client_fd, 0);
  writeVarInt(client_fd, 0);

  // sky light array
  writeVarInt(client_fd, 26);
  for (int i = 0; i < 2048; i ++) chunk_section[i] = 0xFF;
  for (int i = 2048; i < 4096; i ++) chunk_section[i] = 0;
  for (int i = 0; i < 8; i ++) {
    writeVarInt(client_fd, 2048);
    send_all(client_fd, chunk_section + 2048, 2048);
  }
  for (int i = 0; i < 18; i ++) {
    writeVarInt(client_fd, 2048);
    send_all(client_fd, chunk_section, 2048);
  }
  // don't send block light
  writeVarInt(client_fd, 0);

  // Sending block updates changes light prediciton on the client.
  // Light-emitting blocks are omitted from chunk data so that they can
  // be overlayed here. This seems to be cheaper than sending actual
  // block light data.
  for (int i = 0; i < block_changes_count; i ++) {
    #ifdef ALLOW_CHESTS
      if (block_changes[i].block != B_torch && block_changes[i].block != B_chest) continue;
    #else
      if (block_changes[i].block != B_torch) continue;
    #endif
    if (block_changes[i].x < x || block_changes[i].x >= x + 16) continue;
    if (block_changes[i].z < z || block_changes[i].z >= z + 16) continue;
    sc_blockUpdate(client_fd, block_changes[i].x, block_changes[i].y, block_changes[i].z, block_changes[i].block);
  }

  return 0;

}

// S->C Clientbound Keep Alive (play)
int sc_keepAlive (int client_fd) {

  writeVarInt(client_fd, 9);
  writeByte(client_fd, 0x26);

  writeUint64(client_fd, 0);

  return 0;
}

// S->C Set Container Slot
int sc_setContainerSlot (int client_fd, int window_id, uint16_t slot, uint8_t count, uint16_t item) {

  writeVarInt(client_fd,
    1 +
    sizeVarInt(window_id) +
    1 + 2 +
    sizeVarInt(count) +
    (count > 0 ? sizeVarInt(item) + 2 : 0)
  );
  writeByte(client_fd, 0x14);

  writeVarInt(client_fd, window_id);
  writeVarInt(client_fd, 0);
  writeUint16(client_fd, slot);

  writeVarInt(client_fd, count);
  if (count > 0) {
    writeVarInt(client_fd, item);
    writeVarInt(client_fd, 0);
    writeVarInt(client_fd, 0);
  }

  return 0;

}

// S->C Block Update
int sc_blockUpdate (int client_fd, int64_t x, int64_t y, int64_t z, uint8_t block) {
  writeVarInt(client_fd, 9 + sizeVarInt(block_palette[block]));
  writeByte(client_fd, 0x08);
  writeUint64(client_fd, ((x & 0x3FFFFFF) << 38) | ((z & 0x3FFFFFF) << 12) | (y & 0xFFF));
  writeVarInt(client_fd, block_palette[block]);
  return 0;
}

// S->C Acknowledge Block Change
int sc_acknowledgeBlockChange (int client_fd, int sequence) {
  writeVarInt(client_fd, 1 + sizeVarInt(sequence));
  writeByte(client_fd, 0x04);
  writeVarInt(client_fd, sequence);
  return 0;
}

// C->S Player Action
int cs_playerAction (int client_fd) {

  uint8_t action = readByte(client_fd);

  int64_t pos = readInt64(client_fd);
  int x = pos >> 38;
  int y = pos << 52 >> 52;
  int z = pos << 26 >> 38;

  readByte(client_fd); // ignore face

  int sequence = readVarInt(client_fd);
  sc_acknowledgeBlockChange(client_fd, sequence);

  PlayerData *player;
  if (getPlayerData(client_fd, &player)) return 1;

  handlePlayerAction(player, action, x, y, z);

  return 0;

}

// S->C Open Screen
int sc_openScreen (int client_fd, uint8_t window, const char *title, uint16_t length) {

  writeVarInt(client_fd, 1 + 2 * sizeVarInt(window) + 1 + 2 + length);
  writeByte(client_fd, 0x34);

  writeVarInt(client_fd, window);
  writeVarInt(client_fd, window);

  writeByte(client_fd, 8); // string nbt tag
  writeUint16(client_fd, length); // string length
  send_all(client_fd, title, length);

  return 0;
}

// C->S Use Item
int cs_useItem (int client_fd) {

  uint8_t hand = readByte(client_fd);
  int sequence = readVarInt(client_fd);

  // Ignore yaw/pitch
  recv_all(client_fd, recv_buffer, 8, false);

  PlayerData *player;
  if (getPlayerData(client_fd, &player)) return 1;

  handlePlayerUseItem(player, 0, 0, 0, 255);

  return 0;
}

// C->S Use Item On
int cs_useItemOn (int client_fd) {

  uint8_t hand = readByte(client_fd);

  int64_t pos = readInt64(client_fd);
  int x = pos >> 38;
  int y = pos << 52 >> 52;
  int z = pos << 26 >> 38;

  uint8_t face = readByte(client_fd);

  // ignore cursor position
  readUint32(client_fd);
  readUint32(client_fd);
  readUint32(client_fd);

  // ignore "inside block" and "world border hit"
  readByte(client_fd);
  readByte(client_fd);

  int sequence = readVarInt(client_fd);
  sc_acknowledgeBlockChange(client_fd, sequence);

  PlayerData *player;
  if (getPlayerData(client_fd, &player)) return 1;

  handlePlayerUseItem(player, x, y, z, face);

  return 0;
}

// C->S Click Container
int cs_clickContainer (int client_fd) {

  int window_id = readVarInt(client_fd);

  readVarInt(client_fd); // ignore state id

  int16_t clicked_slot = readInt16(client_fd);
  uint8_t button = readByte(client_fd);
  uint8_t mode = readVarInt(client_fd);

  int changes_count = readVarInt(client_fd);

  PlayerData *player;
  if (getPlayerData(client_fd, &player)) return 1;

  uint8_t apply_changes = true;
  // prevent dropping items
  if (mode == 4 && clicked_slot != -999) {
    // when using drop button, re-sync the respective slot
    uint8_t slot = clientSlotToServerSlot(window_id, clicked_slot);
    sc_setContainerSlot(client_fd, window_id, clicked_slot, player->inventory_count[slot], player->inventory_items[slot]);
    apply_changes = false;
  } else if (mode == 0 && clicked_slot == -999) {
    // when clicking outside inventory, return the dropped item to the player
    if (button == 0) {
      givePlayerItem(player, player->flagval_16, player->flagval_8);
      player->flagval_16 = 0;
      player->flagval_8 = 0;
    } else {
      givePlayerItem(player, player->flagval_16, 1);
      player->flagval_8 -= 1;
      if (player->flagval_8 == 0) player->flagval_16 = 0;
    }
    apply_changes = false;
  }

  uint8_t slot, count, craft = false;
  uint16_t item;
  int tmp;

  uint16_t *p_item;
  uint8_t *p_count;

  #ifdef ALLOW_CHESTS
  // See the handlePlayerUseItem function for more info on this hack
  uint8_t *storage_ptr;
  memcpy(&storage_ptr, player->craft_items, sizeof(storage_ptr));
  #endif

  for (int i = 0; i < changes_count; i ++) {

    slot = clientSlotToServerSlot(window_id, readUint16(client_fd));
    // slots outside of the inventory overflow into the crafting buffer
    if (slot > 40 && apply_changes) craft = true;

    #ifdef ALLOW_CHESTS
    if (window_id == 2 && slot > 40) {
      // Get item pointers from the player's storage pointer
      // See the handlePlayerUseItem function for more info on this hack
      p_item = (uint16_t *)(storage_ptr + (slot - 41) * 3);
      p_count = storage_ptr + (slot - 41) * 3 + 2;
    } else
    #endif
    {
      p_item = &player->inventory_items[slot];
      p_count = &player->inventory_count[slot];
    }

    if (!readByte(client_fd)) { // no item?
      if (slot != 255 && apply_changes) {
        *p_item = 0;
        *p_count = 0;
        #ifdef ALLOW_CHESTS
        if (window_id == 2 && slot > 40) {
          broadcastChestUpdate(client_fd, storage_ptr, 0, 0, slot - 41);
        }
        #endif
      }
      continue;
    }

    item = readVarInt(client_fd);
    count = (uint8_t)readVarInt(client_fd);

    // ignore components
    tmp = readVarInt(client_fd);
    recv_all(client_fd, recv_buffer, tmp, false);
    tmp = readVarInt(client_fd);
    recv_all(client_fd, recv_buffer, tmp, false);

    if (count > 0 && apply_changes) {
      *p_item = item;
      *p_count = count;
      #ifdef ALLOW_CHESTS
      if (window_id == 2 && slot > 40) {
        broadcastChestUpdate(client_fd, storage_ptr, item, count, slot - 41);
      }
      #endif
    }

  }

  // window 0 is player inventory, window 12 is crafting table
  if (craft && (window_id == 0 || window_id == 12)) {
    getCraftingOutput(player, &count, &item);
    sc_setContainerSlot(client_fd, window_id, 0, count, item);
  } else if (window_id == 14) { // furnace
    getSmeltingOutput(player);
    for (int i = 0; i < 3; i ++) {
      sc_setContainerSlot(client_fd, window_id, i, player->craft_count[i], player->craft_items[i]);
    }
  }

  // assign cursor-carried item slot
  if (readByte(client_fd)) {
    player->flagval_16 = readVarInt(client_fd);
    player->flagval_8 = readVarInt(client_fd);
    // ignore components
    tmp = readVarInt(client_fd);
    recv_all(client_fd, recv_buffer, tmp, false);
    tmp = readVarInt(client_fd);
    recv_all(client_fd, recv_buffer, tmp, false);
  } else {
    player->flagval_16 = 0;
    player->flagval_8 = 0;
  }

  return 0;

}

// S->C Set Cursor Item
int sc_setCursorItem (int client_fd, uint16_t item, uint8_t count) {

  writeVarInt(client_fd, 1 + sizeVarInt(count) + (count != 0 ? sizeVarInt(item) + 2 : 0));
  writeByte(client_fd, 0x59);

  writeVarInt(client_fd, count);
  if (count == 0) return 0;

  writeVarInt(client_fd, item);

  // Skip components
  writeByte(client_fd, 0);
  writeByte(client_fd, 0);

  return 0;
}

// C->S Set Player Position And Rotation
int cs_setPlayerPositionAndRotation (int client_fd, double *x, double *y, double *z, float *yaw, float *pitch, uint8_t *on_ground) {

  *x = readDouble(client_fd);
  *y = readDouble(client_fd);
  *z = readDouble(client_fd);

  *yaw = readFloat(client_fd);
  *pitch = readFloat(client_fd);

  *on_ground = readByte(client_fd) & 0x01;

  return 0;
}

// C->S Set Player Position
int cs_setPlayerPosition (int client_fd, double *x, double *y, double *z, uint8_t *on_ground) {

  *x = readDouble(client_fd);
  *y = readDouble(client_fd);
  *z = readDouble(client_fd);

  *on_ground = readByte(client_fd) & 0x01;

  return 0;
}

// C->S Set Player Rotation
int cs_setPlayerRotation (int client_fd, float *yaw, float *pitch, uint8_t *on_ground) {

  *yaw = readFloat(client_fd);
  *pitch = readFloat(client_fd);

  *on_ground = readByte(client_fd) & 0x01;

  return 0;
}

int cs_setPlayerMovementFlags (int client_fd, uint8_t *on_ground) {

  *on_ground = readByte(client_fd) & 0x01;

  PlayerData *player;
  if (!getPlayerData(client_fd, &player))
    broadcastPlayerMetadata(player);

  return 0;
}

// C->S Swing Arm (serverbound)
int cs_swingArm (int client_fd) {

  PlayerData *player;
  if (getPlayerData(client_fd, &player)) return 1;

  uint8_t hand = readVarInt(client_fd);

  uint8_t animation = 255;
  switch (hand) {
    case 0: {
      animation = 0;
      break;
    }
    case 1: {
      animation = 2;
      break;
    }
  }

  if (animation == 255)
    return 1;

  // Forward animation to all connected players
  for (int i = 0; i < MAX_PLAYERS; i ++) {
    PlayerData* other_player = &player_data[i];

    if (other_player->client_fd == -1) continue;
    if (other_player->client_fd == player->client_fd) continue;
    if (other_player->flags & 0x20) continue;

    sc_entityAnimation(other_player->client_fd, player->client_fd, animation);
  }

  return 0;
}

// C->S Set Held Item (serverbound)
int cs_setHeldItem (int client_fd) {

  PlayerData *player;
  if (getPlayerData(client_fd, &player)) return 1;

  player->hotbar = (uint8_t)readUint16(client_fd);

  return 0;
}

// S->C Set Held Item (clientbound)
int sc_setHeldItem (int client_fd, uint8_t slot) {

  writeVarInt(client_fd, sizeVarInt(0x62) + 1);
  writeVarInt(client_fd, 0x62);

  writeByte(client_fd, slot);

  return 0;
}

// C->S Close Container (serverbound)
int cs_closeContainer (int client_fd) {

  uint8_t window_id = readVarInt(client_fd);

  PlayerData *player;
  if (getPlayerData(client_fd, &player)) return 1;

  // return all items in crafting slots to the player
  // or, in the case of chests, simply clear the storage pointer
  for (uint8_t i = 0; i < 9; i ++) {
    if (window_id != 2) {
      givePlayerItem(player, player->craft_items[i], player->craft_count[i]);
      uint8_t client_slot = serverSlotToClientSlot(window_id, 41 + i);
      if (client_slot != 255) sc_setContainerSlot(player->client_fd, window_id, client_slot, 0, 0);
    }
    player->craft_items[i] = 0;
    player->craft_count[i] = 0;
  }

  givePlayerItem(player, player->flagval_16, player->flagval_8);
  sc_setCursorItem(client_fd, 0, 0);
  player->flagval_16 = 0;
  player->flagval_8 = 0;

  return 0;
}

// S->C Player Info Update, "Add Player" action
int sc_playerInfoUpdateAddPlayer (int client_fd, PlayerData player) {

  writeVarInt(client_fd, 21 + strlen(player.name)); // Packet length
  writeByte(client_fd, 0x3F); // Packet ID

  writeByte(client_fd, 0x01); // EnumSet: Add Player
  writeByte(client_fd, 1); // Player count (1 per packet)

  // Player UUID
  send_all(client_fd, player.uuid, 16);
  // Player name
  writeByte(client_fd, strlen(player.name));
  send_all(client_fd, player.name, strlen(player.name));
  // Properties (don't send any)
  writeByte(client_fd, 0);

  return 0;
}

// S->C Spawn Entity
int sc_spawnEntity (
  int client_fd,
  int id, uint8_t *uuid, int type,
  double x, double y, double z,
  uint8_t yaw, uint8_t pitch
) {

  writeVarInt(client_fd, 51 + sizeVarInt(id) + sizeVarInt(type));
  writeByte(client_fd, 0x01);

  writeVarInt(client_fd, id); // Entity ID
  send_all(client_fd, uuid, 16); // Entity UUID
  writeVarInt(client_fd, type); // Entity type

  // Position
  writeDouble(client_fd, x);
  writeDouble(client_fd, y);
  writeDouble(client_fd, z);

  // Angles
  writeByte(client_fd, pitch);
  writeByte(client_fd, yaw);
  writeByte(client_fd, yaw);

  // Data - mostly unused
  writeByte(client_fd, 0);

  // Velocity
  writeUint16(client_fd, 0);
  writeUint16(client_fd, 0);
  writeUint16(client_fd, 0);

  return 0;
}

// S->C Set Entity Metadata
int sc_setEntityMetadata (int client_fd, int id, EntityData *metadata, size_t length) {
  int entity_metadata_size = sizeEntityMetadata(metadata, length);
  if (entity_metadata_size == -1) return 1;

  writeVarInt(client_fd, 2 + sizeVarInt(id) + entity_metadata_size);
  writeByte(client_fd, 0x5C);

  writeVarInt(client_fd, id); // Entity ID

  for (size_t i = 0; i < length; i ++) {
    EntityData *data = &metadata[i];
    writeEntityData(client_fd, data);
  }

  writeByte(client_fd, 0xFF); // End

  return 0;
}

// S->C Spawn Entity (from PlayerData)
int sc_spawnEntityPlayer (int client_fd, PlayerData player) {
  return sc_spawnEntity(
    client_fd,
    player.client_fd, player.uuid, 149,
    player.x > 0 ? (double)player.x + 0.5 : (double)player.x - 0.5,
    player.y,
    player.z > 0 ? (double)player.z + 0.5 : (float)player.z - 0.5,
    player.yaw, player.pitch
  );
}

// S->C Entity Animation
int sc_entityAnimation (int client_fd, int id, uint8_t animation) {
  writeVarInt(client_fd, 2 + sizeVarInt(id));
  writeByte(client_fd, 0x02);

  writeVarInt(client_fd, id); // Entity ID
  writeByte(client_fd, animation); // Animation

  return 0;
}

// S->C Teleport Entity
int sc_teleportEntity (
  int client_fd, int id,
  double x, double y, double z,
  float yaw, float pitch
) {

  // Packet length and ID
  writeVarInt(client_fd, 58 + sizeVarInt(id));
  writeByte(client_fd, 0x1F);

  // Entity ID
  writeVarInt(client_fd, id);
  // Position
  writeDouble(client_fd, x);
  writeDouble(client_fd, y);
  writeDouble(client_fd, z);
  // Velocity
  writeUint64(client_fd, 0);
  writeUint64(client_fd, 0);
  writeUint64(client_fd, 0);
  // Angles
  writeFloat(client_fd, yaw);
  writeFloat(client_fd, pitch);
  // On ground flag
  writeByte(client_fd, 1);

  return 0;
}

// S->C Set Head Rotation
int sc_setHeadRotation (int client_fd, int id, uint8_t yaw) {

  // Packet length and ID
  writeByte(client_fd, 2 + sizeVarInt(id));
  writeByte(client_fd, 0x4C);
  // Entity ID
  writeVarInt(client_fd, id);
  // Head yaw
  writeByte(client_fd, yaw);

  return 0;
}

// S->C Set Head Rotation
int sc_updateEntityRotation (int client_fd, int id, uint8_t yaw, uint8_t pitch) {

  // Packet length and ID
  writeByte(client_fd, 4 + sizeVarInt(id));
  writeByte(client_fd, 0x31);
  // Entity ID
  writeVarInt(client_fd, id);
  // Angles
  writeByte(client_fd, yaw);
  writeByte(client_fd, pitch);
  // "On ground" flag
  writeByte(client_fd, 1);

  return 0;
}

// S->C Damage Event
int sc_damageEvent (int client_fd, int entity_id, int type) {

  writeVarInt(client_fd, 4 + sizeVarInt(entity_id) + sizeVarInt(type));
  writeByte(client_fd, 0x19);

  writeVarInt(client_fd, entity_id);
  writeVarInt(client_fd, type);
  writeByte(client_fd, 0);
  writeByte(client_fd, 0);
  writeByte(client_fd, false);

  return 0;
}

// S->C Set Health
int sc_setHealth (int client_fd, uint8_t health, uint8_t food, uint16_t saturation) {

  writeVarInt(client_fd, 9 + sizeVarInt(food));
  writeByte(client_fd, 0x61);

  writeFloat(client_fd, (float)health);
  writeVarInt(client_fd, food);
  writeFloat(client_fd, (float)(saturation - 200) / 500.0f);

  return 0;
}

// S->C Respawn
int sc_respawn (int client_fd) {

  writeVarInt(client_fd, 28);
  writeByte(client_fd, 0x4B);

  // dimension id
  writeVarInt(client_fd, 0);
  // dimension name
  const char *dimension = "overworld";
  writeVarInt(client_fd, 9);
  send_all(client_fd, dimension, 9);
  // hashed seed
  writeUint64(client_fd, 0x0123456789ABCDEF);
  // gamemode
  writeByte(client_fd, GAMEMODE);
  // previous gamemode
  writeByte(client_fd, 0xFF);
  // is debug
  writeByte(client_fd, 0);
  // is flat
  writeByte(client_fd, 0);
  // has death location
  writeByte(client_fd, 0);
  // portal cooldown
  writeVarInt(client_fd, 0);
  // sea level
  writeVarInt(client_fd, 63);
  // data kept
  writeByte(client_fd, 0);

  return 0;
}

// C->S Client Status
int cs_clientStatus (int client_fd) {

  uint8_t id = readByte(client_fd);

  PlayerData *player;
  if (getPlayerData(client_fd, &player)) return 1;

  if (id == 0) {
    sc_respawn(client_fd);
    resetPlayerData(player);
    spawnPlayer(player);
  }

  return 0;
}

// S->C System Chat
int sc_systemChat (int client_fd, char* message, uint16_t len) {

  writeVarInt(client_fd, 5 + len);
  writeByte(client_fd, 0x72);

  // String NBT tag
  writeByte(client_fd, 8);
  writeUint16(client_fd, len);
  send_all(client_fd, message, len);

  // Is action bar message?
  writeByte(client_fd, false);

  return 0;
}

// C->S Chat Message
int cs_chat (int client_fd) {

  readString(client_fd);

  PlayerData *player;
  if (getPlayerData(client_fd, &player)) return 1;

  size_t message_len = strlen((char *)recv_buffer);
  uint8_t name_len = strlen(player->name);

  // To be safe, cap messages to 32 bytes before the buffer length
  if (message_len > 224) {
    recv_buffer[224] = '\0';
    message_len = 224;
  }

  // Shift message contents forward to make space for player name tag
  memmove(recv_buffer + name_len + 3, recv_buffer, message_len + 1);
  // Copy player name to index 1
  memcpy(recv_buffer + 1, player->name, name_len);
  // Surround player name with brackets and a space
  recv_buffer[0] = '<';
  recv_buffer[name_len + 1] = '>';
  recv_buffer[name_len + 2] = ' ';

  // Forward message to all connected players
  for (int i = 0; i < MAX_PLAYERS; i ++) {
    if (player_data[i].client_fd == -1) continue;
    if (player_data[i].flags & 0x20) continue;
    sc_systemChat(player_data[i].client_fd, (char *)recv_buffer, message_len + name_len + 3);
  }

  readUint64(client_fd); // Ignore timestamp
  readUint64(client_fd); // Ignore salt

  // Ignore signature (if any)
  uint8_t has_signature = readByte(client_fd);
  if (has_signature) recv_all(client_fd, recv_buffer, 256, false);

  readVarInt(client_fd); // Ignore message count
  // Ignore acknowledgement bitmask and checksum
  recv_all(client_fd, recv_buffer, 4, false);

  return 0;
}

// C->S Interact
int cs_interact (int client_fd) {

  int entity_id = readVarInt(client_fd);
  uint8_t type = readByte(client_fd);

  if (type == 2) {
    // Ignore target coordinates
    recv_all(client_fd, recv_buffer, 12, false);
  }
  if (type != 1) {
    // Ignore hand
    recv_all(client_fd, recv_buffer, 1, false);
  }

  // Ignore sneaking flag
  recv_all(client_fd, recv_buffer, 1, false);

  if (type == 1) {
    hurtEntity(entity_id, client_fd, D_generic, 1);
  }

  return 0;
}

// S->C Entity Event
int sc_entityEvent (int client_fd, int entity_id, uint8_t status) {

  writeVarInt(client_fd, 6);
  writeByte(client_fd, 0x1E);

  writeUint32(client_fd, entity_id);
  writeByte(client_fd, status);

  return 0;
}

// S->C Remove Entities, but for only one entity per packet
int sc_removeEntity (int client_fd, int entity_id) {

  writeVarInt(client_fd, 2 + sizeVarInt(entity_id));
  writeByte(client_fd, 0x46);

  writeByte(client_fd, 1);
  writeVarInt(client_fd, entity_id);

  return 0;
}

// C->S Player Input
int cs_playerInput (int client_fd) {

  uint8_t flags = readByte(client_fd);

  PlayerData *player;
  if (getPlayerData(client_fd, &player)) return 1;

  // Set or clear sneaking flag
  if (flags & 0x20) player->flags |= 0x04;
  else player->flags &= ~0x04;

  broadcastPlayerMetadata(player);

  return 0;
}

// C->S Player Command
int cs_playerCommand (int client_fd) {

  readVarInt(client_fd); // Ignore entity ID
  uint8_t action = readByte(client_fd);
  readVarInt(client_fd); // Ignore "Jump Boost" value

  PlayerData *player;
  if (getPlayerData(client_fd, &player)) return 1;

  // Handle sprinting
  if (action == 1) player->flags |= 0x08;
  else if (action == 2) player->flags &= ~0x08;

  broadcastPlayerMetadata(player);

  return 0;
}

// S->C Pickup Item (take_item_entity)
int sc_pickupItem (int client_fd, int collected, int collector, uint8_t count) {

  writeVarInt(client_fd, 1 + sizeVarInt(collected) + sizeVarInt(collector) + sizeVarInt(count));
  writeByte(client_fd, 0x75);

  writeVarInt(client_fd, collected);
  writeVarInt(client_fd, collector);
  writeVarInt(client_fd, count);

  return 0;
}

// C->S Player Loaded
int cs_playerLoaded (int client_fd) {

  PlayerData *player;
  if (getPlayerData(client_fd, &player)) return 1;

  // Redirect handling to player join procedure
  handlePlayerJoin(player);

  return 0;
}

// S->C Registry Data (multiple packets) and Update Tags (configuration, multiple packets)
int sc_registries (int client_fd) {

  printf("Sending Registries\n\n");
  send_all(client_fd, registries_bin, sizeof(registries_bin));

  printf("Sending Tags\n\n");
  send_all(client_fd, tags_bin, sizeof(tags_bin));

  return 0;

}
