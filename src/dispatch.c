#include <stdio.h>
#include <string.h>
#include "globals.h"
#include "tools.h"
#include "packets.h"
#include "worldgen.h"
#include "registries.h"
#include "procedures.h"
#include "serialize.h"
#include "dispatch.h"

static void handleHandshakePacket(ServerContext *ctx, int client_fd, int length, int packet_id) {
	switch (packet_id) {
		case 0x00:
			if (cs_handshake(ctx, client_fd)) break;
			break;
		default:
			#ifdef DEV_LOG_UNKNOWN_PACKETS
				printf("Unknown packet: 0x");
				if (packet_id < 16) printf("0");
				printf("%X, length: %d, state: %d\n\n", packet_id, length, getClientState(ctx, client_fd));
			#endif
			recv_all(client_fd, ctx->recv_buffer, length, false);
			break;
	}
}

static void handleStatusPacket(ServerContext *ctx, int client_fd, int length, int packet_id) {
	switch (packet_id) {
		case 0x00:
			if (sc_statusResponse(ctx, client_fd)) break;
			break;
		case 0x01:
			writeByte(client_fd, 9);
			writeByte(client_fd, 0x01);
			writeUint64(client_fd, readUint64(ctx, client_fd));
			ctx->recv_count = 0;
			return;
		default:
				#ifdef DEV_LOG_UNKNOWN_PACKETS
					printf("Unknown packet: 0x");
					if (packet_id < 16) printf("0");
					printf("%X, length: %d, state: %d\n\n", packet_id, length, getClientState(ctx, client_fd));
				#endif
			recv_all(client_fd, ctx->recv_buffer, length, false);
			break;
	}
}

static void handleLoginPacket(ServerContext *ctx, int client_fd, int length, int packet_id) {
	switch (packet_id) {
		case 0x00: {
			uint8_t uuid[16];
			char name[16];
			if (cs_loginStart(ctx, client_fd, uuid, name)) break;
			if (reservePlayerData(ctx, client_fd, uuid, name)) {
				ctx->recv_count = 0;
				return;
			}
			if (sc_loginSuccess(client_fd, uuid, name)) break;
			break;
		}
		case 0x03:
			printf("Client Acknowledged Login\n\n");
			setClientState(ctx, client_fd, STATE_CONFIGURATION);
			break;
		default:
				#ifdef DEV_LOG_UNKNOWN_PACKETS
					printf("Unknown packet: 0x");
					if (packet_id < 16) printf("0");
					printf("%X, length: %d, state: %d\n\n", packet_id, length, getClientState(ctx, client_fd));
				#endif
			recv_all(client_fd, ctx->recv_buffer, length, false);
			break;
	}
}

static void handleConfigurationPacket(ServerContext *ctx, int client_fd, int length, int packet_id) {
	switch (packet_id) {
		case 0x00:
			if (cs_clientInformation(ctx, client_fd)) break;
			if (sc_knownPacks(client_fd)) break;
			if (sc_registries(client_fd)) break;
			#ifdef SEND_BRAND
			if (sc_sendPluginMessage(client_fd, "minecraft:brand", (uint8_t *)ctx->brand, ctx->brand_len)) break;
			#endif
			break;
		case 0x02:
			cs_pluginMessage(ctx, client_fd);
			break;
		case 0x03:
			printf("Client Acknowledged Configuration\n\n");
			completePlayerSpawnSequence(ctx, client_fd);
			break;
		case 0x07:
			printf("Received Client's Known Packs\n");
			printf("  Finishing configuration\n\n");
			sc_finishConfiguration(client_fd);
			completePlayerSpawnSequence(ctx, client_fd);
			break;
		default:
				#ifdef DEV_LOG_UNKNOWN_PACKETS
					printf("Unknown packet: 0x");
					if (packet_id < 16) printf("0");
					printf("%X, length: %d, state: %d\n\n", packet_id, length, getClientState(ctx, client_fd));
				#endif
			recv_all(client_fd, ctx->recv_buffer, length, false);
			break;
	}
}

static void handlePlayPacket(ServerContext *ctx, int client_fd, int length, int packet_id) {
	switch (packet_id) {
		case 0x06:
		case 0x07:
			cs_chatCommand(ctx, client_fd);
			break;
		case 0x08:
			cs_chat(ctx, client_fd);
			break;
		case 0x0B:
			cs_clientStatus(ctx, client_fd);
			break;
		case 0x0C:
			break;
		case 0x11:
			cs_clickContainer(ctx, client_fd);
			break;
		case 0x12:
			cs_closeContainer(ctx, client_fd);
			break;
		case 0x1B:
			recv_all(client_fd, ctx->recv_buffer, length, false);
			break;
		case 0x19:
			cs_interact(ctx, client_fd);
			break;
		case 0x1D:
		case 0x1E:
		case 0x1F:
		case 0x20: {
			double x, y, z; float yaw, pitch; uint8_t on_ground;
			if (packet_id == 0x1D) cs_setPlayerPosition(ctx, client_fd, &x, &y, &z, &on_ground);
			else if (packet_id == 0x1F) cs_setPlayerRotation(ctx, client_fd, &yaw, &pitch, &on_ground);
			else if (packet_id == 0x20) cs_setPlayerMovementFlags(ctx, client_fd, &on_ground);
			else cs_setPlayerPositionAndRotation(ctx, client_fd, &x, &y, &z, &yaw, &pitch, &on_ground);

			PlayerData *player; if (getPlayerData(ctx, client_fd, &player)) break;
			uint8_t block_feet = getBlockAt(ctx, player->x, player->y, player->z);
			uint8_t swimming = block_feet >= B_water && block_feet < B_water + 8;
			if (on_ground) {
				int16_t damage = player->grounded_y - player->y - 3;
				if (damage > 0 && (GAMEMODE == 0 || GAMEMODE == 2) && !swimming) {
					hurtEntity(ctx, client_fd, -1, D_fall, damage);
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
				if (++player->packets_since_update < ctx->client_count) {
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
					if (ctx->player_data[i].client_fd == -1) continue;
					if (ctx->player_data[i].flags & 0x20) continue;
					if (ctx->player_data[i].client_fd == client_fd) continue;
					if (packet_id == 0x1F) {
						sc_updateEntityRotation(ctx->player_data[i].client_fd, client_fd, player->yaw, player->pitch);
					} else {
						sc_teleportEntity(ctx->player_data[i].client_fd, client_fd, x, y, z, yaw, pitch);
					}
					sc_setHeadRotation(ctx->player_data[i].client_fd, client_fd, player->yaw);
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
			uint32_t r_mob = fast_rand(ctx);
			if ((r_mob & 3) == 0) {
				short mob_x = (_x + dx * VIEW_DISTANCE) * 16 + ((r_mob >> 4) & 15);
				short mob_z = (_z + dz * VIEW_DISTANCE) * 16 + ((r_mob >> 8) & 15);
				uint8_t mob_y = cy - 8;
				uint8_t b_low = getBlockAt(ctx, mob_x, mob_y - 1, mob_z);
				uint8_t b_mid = getBlockAt(ctx, mob_x, mob_y, mob_z);
				uint8_t b_top = getBlockAt(ctx, mob_x, mob_y + 1, mob_z);
				while (mob_y < 255) {
					if (!isPassableBlock(b_low) && isPassableSpawnBlock(b_mid) && isPassableSpawnBlock(b_top)) break;
					b_low = b_mid; b_mid = b_top; b_top = getBlockAt(ctx, mob_x, mob_y + 2, mob_z); mob_y ++;
				}
				if (mob_y != 255) {
					if ((ctx->world_time < 13000 || ctx->world_time > 23460) && mob_y > 48) {
						uint32_t mob_choice = (r_mob >> 12) & 3;
						if (mob_choice == 0) spawnMob(ctx, 25, mob_x, mob_y, mob_z, 4);
						else if (mob_choice == 1) spawnMob(ctx, 28, mob_x, mob_y, mob_z, 10);
						else if (mob_choice == 2) spawnMob(ctx, 95, mob_x, mob_y, mob_z, 10);
						else if (mob_choice == 3) spawnMob(ctx, 106, mob_x, mob_y, mob_z, 8);
					} else {
						spawnMob(ctx, 145, mob_x, mob_y, mob_z, 20);
					}
				}
			}
			int count = 0;
			sc_setCenterChunk(client_fd, _x, _z);
			while (dx != 0) {
				sc_chunkDataAndUpdateLight(ctx, client_fd, _x + dx * VIEW_DISTANCE, _z);
				count ++;
				for (int i = 1; i <= VIEW_DISTANCE; i ++) {
					sc_chunkDataAndUpdateLight(ctx, client_fd, _x + dx * VIEW_DISTANCE, _z - i);
					sc_chunkDataAndUpdateLight(ctx, client_fd, _x + dx * VIEW_DISTANCE, _z + i);
					count += 2;
				}
				dx += dx > 0 ? -1 : 1;
			}
			while (dz != 0) {
				sc_chunkDataAndUpdateLight(ctx, client_fd, _x, _z + dz * VIEW_DISTANCE);
				count ++;
				for (int i = 1; i <= VIEW_DISTANCE; i ++) {
					sc_chunkDataAndUpdateLight(ctx, client_fd, _x - i, _z + dz * VIEW_DISTANCE);
					sc_chunkDataAndUpdateLight(ctx, client_fd, _x + i, _z + dz * VIEW_DISTANCE);
					count += 2;
				}
				dz += dz > 0 ? -1 : 1;
			}
			break;
		}
		case 0x28:
			cs_playerAction(ctx, client_fd);
			break;
		case 0x29:
			cs_playerCommand(ctx, client_fd);
			break;
		case 0x2A:
			cs_playerInput(ctx, client_fd);
			break;
		case 0x2B:
			cs_playerLoaded(ctx, client_fd);
			break;
		case 0x34:
			cs_setHeldItem(ctx, client_fd);
			break;
		case 0x3C:
			cs_swingArm(ctx, client_fd);
			break;
		case 0x3F:
			cs_useItemOn(ctx, client_fd);
			break;
		case 0x40:
			cs_useItem(ctx, client_fd);
			break;
		default:
			#ifdef DEV_LOG_UNKNOWN_PACKETS
				printf("Unknown packet: 0x");
				if (packet_id < 16) printf("0");
				printf("%X, length: %d, state: %d\n\n", packet_id, length, getClientState(ctx, client_fd));
			#endif
			recv_all(client_fd, ctx->recv_buffer, length, false);
			break;
	}
}

// Handle incoming packets; moved here from old c_main.c
void handlePacket (ServerContext *ctx, int client_fd, int length, int packet_id, int state) {
		uint64_t bytes_received_start = total_bytes_received;

		switch (state) {
			case STATE_NONE:
				handleHandshakePacket(ctx, client_fd, length, packet_id);
				break;
			case STATE_STATUS:
				handleStatusPacket(ctx, client_fd, length, packet_id);
				break;
			case STATE_LOGIN:
				handleLoginPacket(ctx, client_fd, length, packet_id);
				break;
			case STATE_CONFIGURATION:
				handleConfigurationPacket(ctx, client_fd, length, packet_id);
				break;
			case STATE_PLAY:
				handlePlayPacket(ctx, client_fd, length, packet_id);
				break;
			default:
				#ifdef DEV_LOG_UNKNOWN_PACKETS
					printf("Packet received for unknown state: 0x%X\n\n", state);
				#endif
				recv_all(client_fd, ctx->recv_buffer, length, false);
				break;
		}

		int processed_length = total_bytes_received - bytes_received_start;
		if (processed_length == length) return;
		if (length > processed_length) {
			recv_all(client_fd, ctx->recv_buffer, length - processed_length, false);
		}
		#ifdef DEV_LOG_LENGTH_DISCREPANCY
		if (processed_length != 0) {
				printf("WARNING: Packet 0x%X parsed incorrectly!\n  Expected: %d, parsed: %d\n\n", packet_id, length, processed_length);
		}
		#endif
}


