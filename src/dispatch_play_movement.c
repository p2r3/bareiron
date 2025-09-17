#include <stdint.h>
#include "globals.h"
#include "tools.h"
#include "packets.h"
#include "worldgen.h"
#include "registries.h"
#include "procedures.h"
#include "dispatch_play.h"

void handlePlayMovement(ServerContext *ctx, int client_fd, int length, int packet_id) {
  switch (packet_id) {
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
    default:
      recv_all(client_fd, ctx->recv_buffer, length, false);
  }
}
