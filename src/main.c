#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

#ifdef ESP_PLATFORM
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"
  #include "nvs_flash.h"
  #include "esp_wifi.h"
  #include "esp_event.h"
  #include "esp_timer.h"
  #include "lwip/sockets.h"
  #include "lwip/netdb.h"
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <time.h>
#endif

#include "globals.h"
#include "tools.h"
#include "varnum.h"
#include "packets.h"
#include "worldgen.h"
#include "registries.h"
#include "procedures.h"
#include "serialize.h"

/**
 * Routes an incoming packet to its packet handler or procedure.
 *
 * Full disclosure, I think this whole thing is a bit of a mess.
 * The packet handlers started out as having proper error checks and
 * handling, but that turned out to be very tedious and space/time
 * consuming, and didn't really help with resolving errors. Not to mention
 * that all those checks likely compound into a non-negligible performance
 * hit on embedded systems.
 *
 * I think the way forward would be to gut the return values of the packet
 * handlers, as most of them only ever return 0, and others aren't checked
 * here. The length discrepancy checks at the bottom already do a good job
 * at preventing this from derailing completely in case of a bad packet,
 * and I think leaning into those is fine.
 *
 * In other words, I think the sc_/cs_ handlers should be of type `void`,
 * and should simply return early when there's a failure that prevents the
 * server from handling a packet. Any data that's left unhandled/unread
 * will be caught by the length discrepancy checks. That's more or less
 * how it already works, just not explicitly.
 *
 * Why have I not done this yet? Well, I'm close to uploading the video,
 * and I don't want to risk refactoring anything this close to release.
 */
void handlePacket (int client_fd, int length, int packet_id, int state) {

  // Count the amount of bytes received to catch length discrepancies
  uint64_t bytes_received_start = total_bytes_received;

  switch (packet_id) {

    case 0x00:
      if (state == STATE_NONE) {
        if (cs_handshake(client_fd)) break;
      } else if (state == STATE_STATUS) {
        if (sc_statusResponse(client_fd)) break;
      } if (state == STATE_LOGIN) {
        uint8_t uuid[16];
        char name[16];
        if (cs_loginStart(client_fd, uuid, name)) break;
        if (reservePlayerData(client_fd, uuid, name)) {
          recv_count = 0;
          return;
        }
        if (sc_loginSuccess(client_fd, uuid, name)) break;
      } else if (state == STATE_CONFIGURATION) {
        if (cs_clientInformation(client_fd)) break;
        if (sc_knownPacks(client_fd)) break;
        if (sc_registries(client_fd)) break;

        #ifdef SEND_BRAND
        if (sc_sendPluginMessage(client_fd, "minecraft:brand", (uint8_t *)brand, brand_len)) break;
        #endif
      }
      break;

    case 0x01:
      // Handle status ping
      if (state == STATE_STATUS) {
        // No need for a packet handler, just echo back the long verbatim
        writeByte(client_fd, 9);
        writeByte(client_fd, 0x01);
        writeUint64(client_fd, readUint64(client_fd));
        // Close connection after this
        recv_count = 0;
        return;
      }
      break;

    case 0x02:
      if (state == STATE_CONFIGURATION) cs_pluginMessage(client_fd);
      break;

    case 0x03:
      if (state == STATE_LOGIN) {
        printf("Client Acknowledged Login\n\n");
        setClientState(client_fd, STATE_CONFIGURATION);
      } else if (state == STATE_CONFIGURATION) {
        printf("Client Acknowledged Configuration\n\n");

        // Enter client into "play" state
        setClientState(client_fd, STATE_PLAY);
        sc_loginPlay(client_fd);

        PlayerData *player;
        if (getPlayerData(client_fd, &player)) break;

        // Send full client spawn sequence
        spawnPlayer(player);

        // Register all existing players and spawn their entities
        for (int i = 0; i < MAX_PLAYERS; i ++) {
          if (player_data[i].client_fd == -1) continue;
          // Note that this will also filter out the joining player
          if (player_data[i].flags & 0x20) continue;
          sc_playerInfoUpdateAddPlayer(client_fd, player_data[i]);
          sc_spawnEntityPlayer(client_fd, player_data[i]);
        }

        // Send information about all other entities (mobs):
        // Use a random number for the first half of the UUID
        uint8_t uuid[16];
        uint32_t r = fast_rand();
        memcpy(uuid, &r, 4);
        // Send allocated living mobs, use ID for second half of UUID
        for (int i = 0; i < MAX_MOBS; i ++) {
          if (mob_data[i].type == 0) continue;
          if ((mob_data[i].data & 31) == 0) continue;
          memcpy(uuid + 4, &i, 4);
          // For more info on the arguments here, see the spawnMob function
          sc_spawnEntity(
            client_fd, -2 - i, uuid,
            mob_data[i].type, mob_data[i].x, mob_data[i].y, mob_data[i].z,
            0, 0
          );
        }

      }
      break;

    case 0x07:
      if (state == STATE_CONFIGURATION) {
        printf("Received Client's Known Packs\n");
        printf("  Finishing configuration\n\n");
        sc_finishConfiguration(client_fd);
      }
      break;

    case 0x08:
      if (state == STATE_PLAY) cs_chat(client_fd);
      break;

    case 0x0B:
      if (state == STATE_PLAY) cs_clientStatus(client_fd);
      break;

    case 0x0C: // Client tick (ignored)
      break;

    case 0x11:
      if (state == STATE_PLAY) cs_clickContainer(client_fd);
      break;

    case 0x12:
      if (state == STATE_PLAY) cs_closeContainer(client_fd);
      break;

    case 0x1B:
      if (state == STATE_PLAY) {
        // Serverbound keep-alive (ignored)
        recv_all(client_fd, recv_buffer, length, false);
      }
      break;

    case 0x19:
      if (state == STATE_PLAY) cs_interact(client_fd);
      break;

    case 0x1D:
    case 0x1E:
    case 0x1F:
    case 0x20:
      if (state == STATE_PLAY) {

        double x, y, z;
        float yaw, pitch;
        uint8_t on_ground;

        // Read player position (and rotation)
        if (packet_id == 0x1D) cs_setPlayerPosition(client_fd, &x, &y, &z, &on_ground);
        else if (packet_id == 0x1F) cs_setPlayerRotation (client_fd, &yaw, &pitch, &on_ground);
        else if (packet_id == 0x20) cs_setPlayerMovementFlags (client_fd, &on_ground);
        else cs_setPlayerPositionAndRotation(client_fd, &x, &y, &z, &yaw, &pitch, &on_ground);

        PlayerData *player;
        if (getPlayerData(client_fd, &player)) break;

        uint8_t block_feet = getBlockAt(player->x, player->y, player->z);
        uint8_t swimming = block_feet >= B_water && block_feet < B_water + 8;

        // Handle fall damage
        if (on_ground) {
          int16_t damage = player->grounded_y - player->y - 3;
          if (damage > 0 && (GAMEMODE == 0 || GAMEMODE == 2) && !swimming) {
            hurtEntity(client_fd, -1, D_fall, damage);
          }
          player->grounded_y = player->y;
        } else if (swimming) {
          player->grounded_y = player->y;
        }

        // Don't continue if all we got were flags
        if (packet_id == 0x20) break;

        // Update rotation in player data (if applicable)
        if (packet_id != 0x1D) {
          player->yaw = ((short)(yaw + 540) % 360 - 180) * 127 / 180;
          player->pitch = pitch / 90.0f * 127.0f;
        }

        // Whether to broadcast player position to other players
        uint8_t should_broadcast = true;

        #ifndef BROADCAST_ALL_MOVEMENT
          // If applicable, tie movement updates to the tickrate by using
          // a flag that gets reset on every tick. It might sound better
          // to just make the tick handler broadcast position updates, but
          // then we lose precision. While position is stored using integers,
          // here the client gives us doubles and floats directly.
          should_broadcast = !(player->flags & 0x40);
          if (should_broadcast) player->flags |= 0x40;
        #endif

        #ifdef SCALE_MOVEMENT_UPDATES_TO_PLAYER_COUNT
          // If applicable, broadcast only every client_count-th movement update
          if (++player->packets_since_update < client_count) {
            should_broadcast = false;
          } else {
            // Note that this does not explicitly set should_broadcast to true
            // This allows the above BROADCAST_ALL_MOVEMENT check to compound
            // Whether that's ever favorable is up for debate
            player->packets_since_update = 0;
          }
        #endif

        if (should_broadcast) {
          // If the packet had no rotation data, calculate it from player data
          if (packet_id == 0x1D) {
            yaw = player->yaw * 180 / 127;
            pitch = player->pitch * 90 / 127;
          }
          // Send current position data to all connected players
          for (int i = 0; i < MAX_PLAYERS; i ++) {
            if (player_data[i].client_fd == -1) continue;
            if (player_data[i].flags & 0x20) continue;
            if (player_data[i].client_fd == client_fd) continue;
            if (packet_id == 0x1F) {
              sc_updateEntityRotation(player_data[i].client_fd, client_fd, player->yaw, player->pitch);
            } else {
              sc_teleportEntity(player_data[i].client_fd, client_fd, x, y, z, yaw, pitch);
            }
            sc_setHeadRotation(player_data[i].client_fd, client_fd, player->yaw);
          }
        }

        // Don't continue if all we got was rotation data
        if (packet_id == 0x1F) break;

        // Players send movement packets roughly 20 times per second when
        // moving, and much less frequently when standing still. We can
        // use this correlation between actions and packet count to cheaply
        // simulate hunger with a timer-based system, where the timer ticks
        // down with each position packet. The timer value itself then
        // naturally works as a substitute for saturation.
        if (player->saturation == 0) {
          if (player->hunger > 0) player->hunger--;
          player->saturation = 200;
          sc_setHealth(client_fd, player->health, player->hunger, player->saturation);
        } else if (player->flags & 0x08) {
          player->saturation -= 1;
        }

        // Cast the values to short to get integer position
        short cx = x, cy = y, cz = z;
        if (x < 0) cx -= 1;
        if (z < 0) cz -= 1;
        // Determine the player's chunk coordinates
        short _x = (cx < 0 ? cx - 16 : cx) / 16, _z = (cz < 0 ? cz - 16 : cz) / 16;
        // Calculate distance between previous and current chunk coordinates
        short dx = _x - (player->x < 0 ? player->x - 16 : player->x) / 16;
        short dz = _z - (player->z < 0 ? player->z - 16 : player->z) / 16;

        // Prevent players from leaving the world
        if (cy < 0) {
          cy = 0;
          player->grounded_y = 0;
          sc_synchronizePlayerPosition(client_fd, cx, 0, cz, player->yaw * 180 / 127, player->pitch * 90 / 127);
        } else if (cy > 255) {
          cy = 255;
          sc_synchronizePlayerPosition(client_fd, cx, 255, cz, player->yaw * 180 / 127, player->pitch * 90 / 127);
        }

        // Update position in player data
        player->x = cx;
        player->y = cy;
        player->z = cz;

        // Exit early if no chunk borders were crossed
        if (dx == 0 && dz == 0) break;

        // Check if the player has recently been in this chunk
        int found = false;
        for (int i = 0; i < VISITED_HISTORY; i ++) {
          if (player->visited_x[i] == _x && player->visited_z[i] == _z) {
            found = true;
            break;
          }
        }
        if (found) break;

        // Update player's recently visited chunks
        for (int i = 0; i < VISITED_HISTORY - 1; i ++) {
          player->visited_x[i] = player->visited_x[i + 1];
          player->visited_z[i] = player->visited_z[i + 1];
        }
        player->visited_x[VISITED_HISTORY - 1] = _x;
        player->visited_z[VISITED_HISTORY - 1] = _z;

        uint32_t r = fast_rand();
        // One in every 4 new chunks spawns a mob
        if ((r & 3) == 0) {
          // The mob is placed in the middle of the new chunk row,
          // at a random position within the chunk
          short mob_x = (_x + dx * VIEW_DISTANCE) * 16 + ((r >> 4) & 15);
          short mob_z = (_z + dz * VIEW_DISTANCE) * 16 + ((r >> 8) & 15);
          // Start at the Y coordinate of the spawning player and move upward
          // until a valid space is found
          uint8_t mob_y = cy - 8;
          uint8_t b_low = getBlockAt(mob_x, mob_y - 1, mob_z);
          uint8_t b_mid = getBlockAt(mob_x, mob_y, mob_z);
          uint8_t b_top = getBlockAt(mob_x, mob_y + 1, mob_z);
          while (mob_y < 255) {
            if ( // Solid block below, non-solid(spawnable) at feet and above
              !isPassableBlock(b_low) &&
              isPassableSpawnBlock(b_mid) &&
              isPassableSpawnBlock(b_top)
            ) break;
            b_low = b_mid;
            b_mid = b_top;
            b_top = getBlockAt(mob_x, mob_y + 2, mob_z);
            mob_y ++;
          }
          if (mob_y != 255) {
            // Spawn passive mobs above ground during the day,
            // or hostiles underground and during the night
            if ((world_time < 13000 || world_time > 23460) && mob_y > 48) {
              uint32_t mob_choice = (r >> 12) & 3;
              if (mob_choice == 0) spawnMob(25, mob_x, mob_y, mob_z, 4); // Chicken
              else if (mob_choice == 1) spawnMob(28, mob_x, mob_y, mob_z, 10); // Cow
              else if (mob_choice == 2) spawnMob(95, mob_x, mob_y, mob_z, 10); // Pig
              else if (mob_choice == 3) spawnMob(106, mob_x, mob_y, mob_z, 8); // Sheep
            } else {
              spawnMob(145, mob_x, mob_y, mob_z, 20); // Zombie
            }
          }
        }

        int count = 0;
        #ifdef DEV_LOG_CHUNK_GENERATION
          printf("Sending new chunks (%d, %d)\n", _x, _z);
          clock_t start, end;
          start = clock();
        #endif

        sc_setCenterChunk(client_fd, _x, _z);

        while (dx != 0) {
          sc_chunkDataAndUpdateLight(client_fd, _x + dx * VIEW_DISTANCE, _z);
          count ++;
          for (int i = 1; i <= VIEW_DISTANCE; i ++) {
            sc_chunkDataAndUpdateLight(client_fd, _x + dx * VIEW_DISTANCE, _z - i);
            sc_chunkDataAndUpdateLight(client_fd, _x + dx * VIEW_DISTANCE, _z + i);
            count += 2;
          }
          dx += dx > 0 ? -1 : 1;
        }
        while (dz != 0) {
          sc_chunkDataAndUpdateLight(client_fd, _x, _z + dz * VIEW_DISTANCE);
          count ++;
          for (int i = 1; i <= VIEW_DISTANCE; i ++) {
            sc_chunkDataAndUpdateLight(client_fd, _x - i, _z + dz * VIEW_DISTANCE);
            sc_chunkDataAndUpdateLight(client_fd, _x + i, _z + dz * VIEW_DISTANCE);
            count += 2;
          }
          dz += dz > 0 ? -1 : 1;
        }

        #ifdef DEV_LOG_CHUNK_GENERATION
          end = clock();
          double total_ms = (double)(end - start) / CLOCKS_PER_SEC * 1000;
          printf("Generated %d chunks in %.0f ms (%.2f ms per chunk)\n", count, total_ms, total_ms / (double)count);
        #endif

      }
      break;

    case 0x29:
      if (state == STATE_PLAY) cs_playerCommand(client_fd);
      break;

    case 0x2A:
      if (state == STATE_PLAY) cs_playerInput(client_fd);
      break;

    case 0x2B:
      if (state == STATE_PLAY) cs_playerLoaded(client_fd);
      break;

    case 0x34:
      if (state == STATE_PLAY) cs_setHeldItem(client_fd);
      break;
	
    case 0x3C:
      if (state == STATE_PLAY) cs_swingArm(client_fd);
      break;

    case 0x28:
      if (state == STATE_PLAY) cs_playerAction(client_fd);
      break;

    case 0x3F:
      if (state == STATE_PLAY) cs_useItemOn(client_fd);
      break;

    case 0x40:
      if (state == STATE_PLAY) cs_useItem(client_fd);
      break;

    default:
      #ifdef DEV_LOG_UNKNOWN_PACKETS
        printf("Unknown packet: 0x");
        if (packet_id < 16) printf("0");
        printf("%X, length: %d, state: %d\n\n", packet_id, length, state);
      #endif
      recv_all(client_fd, recv_buffer, length, false);
      break;

  }

  // Detect and fix incorrectly parsed packets
  int processed_length = total_bytes_received - bytes_received_start;
  if (processed_length == length) return;

  if (length > processed_length) {
    recv_all(client_fd, recv_buffer, length - processed_length, false);
  }

  #ifdef DEV_LOG_LENGTH_DISCREPANCY
  if (processed_length != 0) {
    printf("WARNING: Packet 0x");
    if (packet_id < 16) printf("0");
    printf("%X parsed incorrectly!\n  Expected: %d, parsed: %d\n\n", packet_id, length, processed_length);
  }
  #endif
  #ifdef DEV_LOG_UNKNOWN_PACKETS
  if (processed_length == 0) {
    printf("Unknown packet: 0x");
    if (packet_id < 16) printf("0");
    printf("%X, length: %d, state: %d\n\n", packet_id, length, state);
  }
  #endif

}

int main () {

  // Hash the seeds to ensure they're random enough
  world_seed = splitmix64(world_seed);
  printf("World seed (hashed): ");
  for (int i = 3; i >= 0; i --) printf("%X", (unsigned int)((world_seed >> (8 * i)) & 255));

  rng_seed = splitmix64(rng_seed);
  printf("\nRNG seed (hashed): ");
  for (int i = 3; i >= 0; i --) printf("%X", (unsigned int)((rng_seed >> (8 * i)) & 255));
  printf("\n\n");

  // Initialize block changes entries as unallocated
  for (int i = 0; i < MAX_BLOCK_CHANGES; i ++) {
    block_changes[i].block = 0xFF;
  }

  // Start the disk/flash serializer (if applicable)
  if (initSerializer()) exit(EXIT_FAILURE);

  // Initialize all file descriptor references to -1 (unallocated)
  int clients[MAX_PLAYERS], client_index = 0;
  for (int i = 0; i < MAX_PLAYERS; i ++) {
    clients[i] = -1;
    client_states[i * 2] = -1;
    player_data[i].client_fd = -1;
  }

  // Create server TCP socket
  int server_fd, opt = 1;
  struct sockaddr_in server_addr, client_addr;
  socklen_t addr_len = sizeof(client_addr);

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("socket options failed");
    exit(EXIT_FAILURE);
  }

  // Bind socket to IP/port
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("bind failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  // Listen for incoming connections
  if (listen(server_fd, 5) < 0) {
    perror("listen failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }
  printf("Server listening on port %d...\n", PORT);

  // Make the socket non-blocking
  // This is necessary to not starve the idle task during slow connections
  int flags = fcntl(server_fd, F_GETFL, 0);
  fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

  // Track time of last server tick (in microseconds)
  int64_t last_tick_time = get_program_time();

  /**
   * Cycles through all connected clients, handling one packet at a time
   * from each player. With every iteration, attempts to accept a new
   * client connection.
   */
  while (true) {
    // Check if it's time to yield to the idle task
    task_yield();

    // Attempt to accept a new connection
    for (int i = 0; i < MAX_PLAYERS; i ++) {
      if (clients[i] != -1) continue;
      clients[i] = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
      // If the accept was successful, make the client non-blocking too
      if (clients[i] != -1) {
        printf("New client, fd: %d\n", clients[i]);
        int flags = fcntl(clients[i], F_GETFL, 0);
        fcntl(clients[i], F_SETFL, flags | O_NONBLOCK);
        client_count ++;
      }
      break;
    }

    // Look for valid connected clients
    client_index ++;
    if (client_index == MAX_PLAYERS) client_index = 0;
    if (clients[client_index] == -1) continue;

    // Handle periodic events (server ticks)
    int64_t time_since_last_tick = get_program_time() - last_tick_time;
    if (time_since_last_tick > TIME_BETWEEN_TICKS) {
      handleServerTick(time_since_last_tick);
      last_tick_time = get_program_time();
    }

    // Handle this individual client
    int client_fd = clients[client_index];

    // Check if at least 2 bytes are available for reading
    recv_count = recv(client_fd, &recv_buffer, 2, MSG_PEEK);
    if (recv_count < 2) {
      if (recv_count == 0 || (recv_count < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
        disconnectClient(&clients[client_index], 1);
      }
      continue;
    }

    // Handle 0xBEEF and 0xFEED packets for dumping/uploading world data
    #ifdef DEV_ENABLE_BEEF_DUMPS
    // Received BEEF packet, dump world data and disconnect
    if (recv_buffer[0] == 0xBE && recv_buffer[1] == 0xEF && getClientState(client_fd) == STATE_NONE) {
      // Send block changes and player data back to back
      // The client is expected to know (or calculate) the size of these buffers
      send_all(client_fd, block_changes, sizeof(block_changes));
      send_all(client_fd, player_data, sizeof(player_data));
      // Flush the socket and receive everything left on the wire
      shutdown(client_fd, SHUT_WR);
      recv_all(client_fd, recv_buffer, sizeof(recv_buffer), false);
      // Kick the client
      disconnectClient(&clients[client_index], 6);
      continue;
    }
    // Received FEED packet, load world data from socket and disconnect
    if (recv_buffer[0] == 0xFE && recv_buffer[1] == 0xED && getClientState(client_fd) == STATE_NONE) {
      // Consume 0xFEED bytes (previous read was just a peek)
      recv_all(client_fd, recv_buffer, 2, false);
      // Write full buffers straight into memory
      recv_all(client_fd, block_changes, sizeof(block_changes), false);
      recv_all(client_fd, player_data, sizeof(player_data), false);
      // Recover block_changes_count
      for (int i = 0; i < MAX_BLOCK_CHANGES; i ++) {
        if (block_changes[i].block == 0xFF) continue;
        if (block_changes[i].block == B_chest) i += 14;
        if (i >= block_changes_count) block_changes_count = i + 1;
      }
      // Update data on disk
      writeBlockChangesToDisk(0, block_changes_count);
      writePlayerDataToDisk();
      // Kick the client
      disconnectClient(&clients[client_index], 7);
      continue;
    }
    #endif

    // Read packet length
    int length = readVarInt(client_fd);
    if (length == VARNUM_ERROR) {
      disconnectClient(&clients[client_index], 2);
      continue;
    }
    // Read packet ID
    int packet_id = readVarInt(client_fd);
    if (packet_id == VARNUM_ERROR) {
      disconnectClient(&clients[client_index], 3);
      continue;
    }
    // Get client connection state
    int state = getClientState(client_fd);
    // Disconnect on legacy server list ping
    if (state == STATE_NONE && length == 254 && packet_id == 122) {
      disconnectClient(&clients[client_index], 5);
      continue;
    }
    // Handle packet data
    handlePacket(client_fd, length - sizeVarInt(packet_id), packet_id, state);
    if (recv_count == 0 || (recv_count == -1 && errno != EAGAIN && errno != EWOULDBLOCK)) {
      disconnectClient(&clients[client_index], 4);
      continue;
    }

  }

  close(server_fd);
  printf("Server closed.\n");

}

#ifdef ESP_PLATFORM

void bareiron_main (void *pvParameters) {
  main();
  vTaskDelete(NULL);
}

static void wifi_event_handler (void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    esp_wifi_connect();
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    printf("Got IP, starting server...\n\n");
    xTaskCreate(bareiron_main, "bareiron", 4096, NULL, 5, NULL);
  }
}

void wifi_init () {
  nvs_flash_init();
  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);

  esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
  esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);

  wifi_config_t wifi_config = {
    .sta = {
      .ssid = WIFI_SSID,
      .password = WIFI_PASS,
      .threshold.authmode = WIFI_AUTH_WPA2_PSK
    }
  };

  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_start();
}

void app_main () {
  esp_timer_early_init();
  wifi_init();
}

#endif
