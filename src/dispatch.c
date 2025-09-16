#include <stdio.h>
#include <string.h>
#include "globals.h"
#include "tools.h"
#include "varnum.h"
#include "packets.h"
#include "worldgen.h"
#include "registries.h"
#include "procedures.h"
#include "serialize.h"
#include "dispatch.h"

// Handle incoming packets; moved here from old c_main.c
void handlePacket (ServerContext *ctx, int client_fd, int length, int packet_id, int state) {
	uint64_t bytes_received_start = total_bytes_received;

	switch (packet_id) {
		case 0x00:
				if (getClientState(client_fd) == STATE_NONE) {
				if (cs_handshake(client_fd)) break;
				} else if (getClientState(client_fd) == STATE_STATUS) {
				if (sc_statusResponse(client_fd)) break;
				} else if (getClientState(client_fd) == STATE_LOGIN) {
				uint8_t uuid[16];
				char name[16];
				if (cs_loginStart(client_fd, uuid, name)) break;
				if (reservePlayerData(client_fd, uuid, name)) {
					recv_count = 0;
					return;
				}
				if (sc_loginSuccess(client_fd, uuid, name)) break;
			} else if (getClientState(client_fd) == STATE_CONFIGURATION) {
				if (cs_clientInformation(client_fd)) break;
				if (sc_knownPacks(client_fd)) break;
				if (sc_registries(client_fd)) break;
				#ifdef SEND_BRAND
				if (sc_sendPluginMessage(client_fd, "minecraft:brand", (uint8_t *)brand, brand_len)) break;
				#endif
			}
			break;

		case 0x01:
			if (getClientState(client_fd) == STATE_STATUS) {
				writeByte(client_fd, 9);
				writeByte(client_fd, 0x01);
				writeUint64(client_fd, readUint64(client_fd));
				recv_count = 0;
				return;
			}
			break;

		case 0x02:
			if (getClientState(client_fd) == STATE_CONFIGURATION) cs_pluginMessage(client_fd);
			break;

		case 0x03:
			if (getClientState(client_fd) == STATE_LOGIN) {
				printf("Client Acknowledged Login\n\n");
				setClientState(client_fd, STATE_CONFIGURATION);
			} else if (getClientState(client_fd) == STATE_CONFIGURATION) {
				printf("Client Acknowledged Configuration\n\n");
				setClientState(client_fd, STATE_PLAY);
				sc_loginPlay(client_fd);
				PlayerData *player;
				if (getPlayerData(client_fd, &player)) break;
				spawnPlayer(player);
				for (int i = 0; i < MAX_PLAYERS; i ++) {
					if (player_data[i].client_fd == -1) continue;
					if (player_data[i].flags & 0x20) continue;
					sc_playerInfoUpdateAddPlayer(client_fd, player_data[i]);
					sc_spawnEntityPlayer(client_fd, player_data[i]);
				}
				uint8_t uuid[16];
				uint32_t r = fast_rand();
				memcpy(uuid, &r, 4);
				for (int i = 0; i < MAX_MOBS; i ++) {
					if (mob_data[i].type == 0) continue;
					if ((mob_data[i].data & 31) == 0) continue;
					memcpy(uuid + 4, &i, 4);
					sc_spawnEntity(client_fd, -2 - i, uuid, mob_data[i].type, mob_data[i].x, mob_data[i].y, mob_data[i].z, 0, 0);
				}
			}
			break;

		case 0x07:
			if (getClientState(client_fd) == STATE_CONFIGURATION) {
				printf("Received Client's Known Packs\n");
				printf("  Finishing configuration\n\n");
				sc_finishConfiguration(client_fd);
			}
			break;

		case 0x08:
			if (getClientState(client_fd) == STATE_PLAY) cs_chat(client_fd);
			break;

		case 0x0B:
			if (getClientState(client_fd) == STATE_PLAY) cs_clientStatus(client_fd);
			break;

		case 0x0C:
			break;

		case 0x11:
			if (getClientState(client_fd) == STATE_PLAY) cs_clickContainer(client_fd);
			break;

		case 0x12:
			if (getClientState(client_fd) == STATE_PLAY) cs_closeContainer(client_fd);
			break;

		case 0x1B:
			if (getClientState(client_fd) == STATE_PLAY) {
				recv_all(client_fd, recv_buffer, length, false);
			}
			break;

		case 0x19:
			if (getClientState(client_fd) == STATE_PLAY) cs_interact(client_fd);
			break;

		case 0x1D:
		case 0x1E:
		case 0x1F:
		case 0x20:
			if (getClientState(client_fd) == STATE_PLAY) {
				double x, y, z; float yaw, pitch; uint8_t on_ground;
				if (packet_id == 0x1D) cs_setPlayerPosition(client_fd, &x, &y, &z, &on_ground);
				else if (packet_id == 0x1F) cs_setPlayerRotation(client_fd, &yaw, &pitch, &on_ground);
				else if (packet_id == 0x20) cs_setPlayerMovementFlags(client_fd, &on_ground);
				else cs_setPlayerPositionAndRotation(client_fd, &x, &y, &z, &yaw, &pitch, &on_ground);
				PlayerData *player; if (getPlayerData(client_fd, &player)) break;
				uint8_t block_feet = getBlockAt(player->x, player->y, player->z);
				uint8_t swimming = block_feet >= B_water && block_feet < B_water + 8;
				if (on_ground) {
					int16_t damage = player->grounded_y - player->y - 3;
					if (damage > 0 && (GAMEMODE == 0 || GAMEMODE == 2) && !swimming) {
						hurtEntity(client_fd, -1, D_fall, damage);
					}
					player->grounded_y = player->y;
				} else if (swimming) {
					player->grounded_y = player->y;
				}
				if (packet_id == 0x20) break;
				if (packet_id != 0x1D) {
					player->yaw = ((short)(yaw + 540) % 360 - 180) * 127 / 180;
					player->pitch = (int8_t)(pitch / 90.0f * 127.0f);
				}
				uint8_t should_broadcast = true;
				#ifndef BROADCAST_ALL_MOVEMENT
					should_broadcast = !(player->flags & 0x40);
					if (should_broadcast) player->flags |= 0x40;
				#endif
				#ifdef SCALE_MOVEMENT_UPDATES_TO_PLAYER_COUNT
					if (++player->packets_since_update < client_count) {
						should_broadcast = false;
					} else {
						player->packets_since_update = 0;
					}
				#endif
				if (should_broadcast) {
					if (packet_id == 0x1D) {
						yaw = player->yaw * 180 / 127;
						pitch = player->pitch * 90 / 127;
					}
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
				if (packet_id == 0x1F) break;
				if (player->saturation == 0) {
					if (player->hunger > 0) player->hunger--;
					player->saturation = 200;
					sc_setHealth(client_fd, player->health, player->hunger, player->saturation);
				} else if (player->flags & 0x08) {
					player->saturation -= 1;
				}
				short cx = x, cy = y, cz = z;
				if (x < 0) cx -= 1; if (z < 0) cz -= 1;
				short _x = (cx < 0 ? cx - 16 : cx) / 16, _z = (cz < 0 ? cz - 16 : cz) / 16;
				short dx = _x - (player->x < 0 ? player->x - 16 : player->x) / 16;
				short dz = _z - (player->z < 0 ? player->z - 16 : player->z) / 16;
				if (cy < 0) { cy = 0; player->grounded_y = 0; sc_synchronizePlayerPosition(client_fd, cx, 0, cz, player->yaw * 180 / 127, player->pitch * 90 / 127); }
				else if (cy > 255) { cy = 255; sc_synchronizePlayerPosition(client_fd, cx, 255, cz, player->yaw * 180 / 127, player->pitch * 90 / 127); }
				player->x = cx; player->y = cy; player->z = cz;
				if (dx == 0 && dz == 0) break;
				int found = false;
				for (int i = 0; i < VISITED_HISTORY; i ++) {
					if (player->visited_x[i] == _x && player->visited_z[i] == _z) { found = true; break; }
				}
				if (found) break;
				for (int i = 0; i < VISITED_HISTORY - 1; i ++) {
					player->visited_x[i] = player->visited_x[i + 1];
					player->visited_z[i] = player->visited_z[i + 1];
				}
				player->visited_x[VISITED_HISTORY - 1] = _x;
				player->visited_z[VISITED_HISTORY - 1] = _z;
				uint32_t r = fast_rand();
				if ((r & 3) == 0) {
					short mob_x = (_x + dx * VIEW_DISTANCE) * 16 + ((r >> 4) & 15);
					short mob_z = (_z + dz * VIEW_DISTANCE) * 16 + ((r >> 8) & 15);
					uint8_t mob_y = cy - 8;
					uint8_t b_low = getBlockAt(mob_x, mob_y - 1, mob_z);
					uint8_t b_mid = getBlockAt(mob_x, mob_y, mob_z);
					uint8_t b_top = getBlockAt(mob_x, mob_y + 1, mob_z);
					while (mob_y < 255) {
						if (!isPassableBlock(b_low) && isPassableSpawnBlock(b_mid) && isPassableSpawnBlock(b_top)) break;
						b_low = b_mid; b_mid = b_top; b_top = getBlockAt(mob_x, mob_y + 2, mob_z); mob_y ++;
					}
					if (mob_y != 255) {
						if ((world_time < 13000 || world_time > 23460) && mob_y > 48) {
							uint32_t mob_choice = (r >> 12) & 3;
							if (mob_choice == 0) spawnMob(25, mob_x, mob_y, mob_z, 4);
							else if (mob_choice == 1) spawnMob(28, mob_x, mob_y, mob_z, 10);
							else if (mob_choice == 2) spawnMob(95, mob_x, mob_y, mob_z, 10);
							else if (mob_choice == 3) spawnMob(106, mob_x, mob_y, mob_z, 8);
						} else {
							spawnMob(145, mob_x, mob_y, mob_z, 20);
						}
					}
				}
				int count = 0;
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
			}
			break;

		case 0x29:
			if (getClientState(client_fd) == STATE_PLAY) cs_playerCommand(client_fd);
			break;
		case 0x2A:
			if (getClientState(client_fd) == STATE_PLAY) cs_playerInput(client_fd);
			break;
		case 0x2B:
			if (getClientState(client_fd) == STATE_PLAY) cs_playerLoaded(client_fd);
			break;
		case 0x34:
			if (getClientState(client_fd) == STATE_PLAY) cs_setHeldItem(client_fd);
			break;
		case 0x3C:
			if (getClientState(client_fd) == STATE_PLAY) cs_swingArm(client_fd);
			break;
		case 0x28:
			if (getClientState(client_fd) == STATE_PLAY) cs_playerAction(client_fd);
			break;
		case 0x3F:
			if (getClientState(client_fd) == STATE_PLAY) cs_useItemOn(client_fd);
			break;
		case 0x40:
			if (getClientState(client_fd) == STATE_PLAY) cs_useItem(client_fd);
			break;
		default:
			#ifdef DEV_LOG_UNKNOWN_PACKETS
				printf("Unknown packet: 0x");
				if (packet_id < 16) printf("0");
				printf("%X, length: %d, state: %d\n\n", packet_id, length, getClientState(client_fd));
			#endif
			recv_all(client_fd, recv_buffer, length, false);
			break;
	}

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
		printf("%X, length: %d, state: %d\n\n", packet_id, length, getClientState(client_fd));
	}
	#endif
}
