const std = @import("std");
const c = @import("c_api.zig").c;
const tools = @import("tools.zig");

pub fn handlePacket(ctx: *c.ServerContext, client_fd: c_int, length: c_int, packet_id: c_int, state: c_int) void {
    _ = state; // unused; we query state from ctx
    const bytes_received_start = tools.total_bytes_received;

    sw: switch (packet_id) {
        0x00 => {
            const current_state = c.getClientState(ctx, client_fd);
            if (current_state == c.STATE_NONE) {
                if (c.cs_handshake(ctx, client_fd) != 0) break :sw;
            } else if (current_state == c.STATE_STATUS) {
                if (c.sc_statusResponse(ctx, client_fd) != 0) break :sw;
            } else if (current_state == c.STATE_LOGIN) {
                var uuid: [16]u8 = undefined;
                var name: [16]u8 = undefined;
                if (c.cs_loginStart(ctx, client_fd, &uuid, &name) != 0) break :sw;
                if (c.reservePlayerData(ctx, client_fd, &uuid, &name) != 0) {
                    ctx.recv_count = 0;
                    return;
                }
                if (c.sc_loginSuccess(client_fd, &uuid, &name) != 0) break :sw;
            } else if (current_state == c.STATE_CONFIGURATION) {
                if (c.cs_clientInformation(ctx, client_fd) != 0) break :sw;
                if (c.sc_knownPacks(client_fd) != 0) break :sw;
                if (c.sc_registries(client_fd) != 0) break :sw;
                if (comptime @hasDecl(c, "SEND_BRAND")) {
                    _ = c.sc_sendPluginMessage(client_fd, "minecraft:brand", @ptrCast(&ctx.brand), ctx.brand_len);
                }
            }
        },
        0x01 => {
            if (c.getClientState(ctx, client_fd) == c.STATE_STATUS) {
                _ = c.writeByte(client_fd, 9);
                _ = c.writeByte(client_fd, 0x01);
                _ = c.writeUint64(client_fd, c.readUint64(ctx, client_fd));
                ctx.recv_count = 0;
                return;
            }
        },
        0x02 => {
            if (c.getClientState(ctx, client_fd) == c.STATE_CONFIGURATION) {
                _ = c.cs_pluginMessage(ctx, client_fd);
            }
        },
        0x03 => {
            const current_state = c.getClientState(ctx, client_fd);
            if (current_state == c.STATE_LOGIN) {
                std.log.info("Client Acknowledged Login", .{});
                c.setClientState(ctx, client_fd, c.STATE_CONFIGURATION);
            } else if (current_state == c.STATE_CONFIGURATION) {
                std.log.info("Client Acknowledged Configuration", .{});
                c.setClientState(ctx, client_fd, c.STATE_PLAY);
                _ = c.sc_loginPlay(client_fd);
                _ = c.sc_commands(client_fd);
                var player_ptr: ?*c.PlayerData = null;
                if (c.getPlayerData(ctx, client_fd, &player_ptr) != 0) break :sw;
                if (player_ptr) |player| {
                    c.spawnPlayer(ctx, player);
                    for (0..c.MAX_PLAYERS) |i| {
                        const other_player = ctx.player_data[i];
                        if (other_player.client_fd == -1 or (other_player.flags & 0x20 != 0)) continue;
                        _ = c.sc_playerInfoUpdateAddPlayer(client_fd, other_player);
                        _ = c.sc_spawnEntityPlayer(client_fd, other_player);
                    }
                    var uuid: [16]u8 = undefined;
                    var r_uuid = c.fast_rand(ctx);
                    @memcpy(uuid[0..4], std.mem.asBytes(&r_uuid));
                    for (0..c.MAX_MOBS) |i| {
                        const mob = ctx.mob_data[i];
                        if (mob.type == 0 or (mob.data & 31) == 0) continue;
                        var mob_idx: c_int = @intCast(i);
                        @memcpy(uuid[4..8], std.mem.asBytes(&mob_idx));
                        _ = c.sc_spawnEntity(
                            client_fd,
                            -2 - mob_idx,
                            &uuid,
                            mob.type,
                            @as(f64, @floatFromInt(mob.x)),
                            @as(f64, @floatFromInt(mob.y)),
                            @as(f64, @floatFromInt(mob.z)),
                            0,
                            0,
                        );
                        c.broadcastMobMetadata(ctx, client_fd, -2 - mob_idx);
                    }
                    c.handlePlayerJoin(ctx, player);
                }
            }
        },
        0x07 => {
            const current_state = c.getClientState(ctx, client_fd);
            if (current_state == c.STATE_CONFIGURATION) {
                std.log.info("Received Client's Known Packs", .{});
                std.log.info("  Finishing configuration", .{});
                _ = c.sc_finishConfiguration(client_fd);
                c.setClientState(ctx, client_fd, c.STATE_PLAY);
                _ = c.sc_loginPlay(client_fd);
                _ = c.sc_commands(client_fd);
                var player_ptr: ?*c.PlayerData = null;
                if (c.getPlayerData(ctx, client_fd, &player_ptr) == 0) {
                    if (player_ptr) |player| {
                        c.spawnPlayer(ctx, player);
                        for (0..c.MAX_PLAYERS) |i| {
                            const other_player = ctx.player_data[i];
                            if (other_player.client_fd == -1 or (other_player.flags & 0x20 != 0)) continue;
                            _ = c.sc_playerInfoUpdateAddPlayer(client_fd, other_player);
                            _ = c.sc_spawnEntityPlayer(client_fd, other_player);
                        }
                        var uuid: [16]u8 = undefined;
                        var r_uuid = c.fast_rand(ctx);
                        @memcpy(uuid[0..4], std.mem.asBytes(&r_uuid));
                        for (0..c.MAX_MOBS) |i| {
                            const mob = ctx.mob_data[i];
                            if (mob.type == 0 or (mob.data & 31) == 0) continue;
                            var mob_idx: c_int = @intCast(i);
                            @memcpy(uuid[4..8], std.mem.asBytes(&mob_idx));
                            _ = c.sc_spawnEntity(
                                client_fd,
                                -2 - mob_idx,
                                &uuid,
                                mob.type,
                                @as(f64, @floatFromInt(mob.x)),
                                @as(f64, @floatFromInt(mob.y)),
                                @as(f64, @floatFromInt(mob.z)),
                                0,
                                0,
                            );
                            c.broadcastMobMetadata(ctx, client_fd, -2 - mob_idx);
                        }
                        c.handlePlayerJoin(ctx, player);
                    }
                }
            } else if (current_state == c.STATE_PLAY) {
                _ = c.cs_chatCommand(ctx, client_fd);
            }
        },
        0x06 => {
            if (c.getClientState(ctx, client_fd) == c.STATE_PLAY) _ = c.cs_chatCommand(ctx, client_fd);
        },
        0x08 => {
            if (c.getClientState(ctx, client_fd) == c.STATE_PLAY) _ = c.cs_chat(ctx, client_fd);
        },
        0x0B => {
            if (c.getClientState(ctx, client_fd) == c.STATE_PLAY) _ = c.cs_clientStatus(ctx, client_fd);
        },
        0x0C => {},
        0x11 => {
            if (c.getClientState(ctx, client_fd) == c.STATE_PLAY) _ = c.cs_clickContainer(ctx, client_fd);
        },
        0x12 => {
            if (c.getClientState(ctx, client_fd) == c.STATE_PLAY) _ = c.cs_closeContainer(ctx, client_fd);
        },
        0x1B => {
            if (c.getClientState(ctx, client_fd) == c.STATE_PLAY) _ = c.recv_all(client_fd, &ctx.recv_buffer, @intCast(length), 0);
        },
        0x19 => {
            if (c.getClientState(ctx, client_fd) == c.STATE_PLAY) _ = c.cs_interact(ctx, client_fd);
        },
        0x1D, 0x1E, 0x1F, 0x20 => {
            if (c.getClientState(ctx, client_fd) == c.STATE_PLAY) {
                var x: f64 = undefined;
                var y: f64 = undefined;
                var z: f64 = undefined;
                var yaw: f32 = undefined;
                var pitch: f32 = undefined;
                var on_ground: u8 = undefined;

                if (packet_id == 0x1D) _ = c.cs_setPlayerPosition(ctx, client_fd, &x, &y, &z, &on_ground);
                if (packet_id == 0x1F) _ = c.cs_setPlayerRotation(ctx, client_fd, &yaw, &pitch, &on_ground);
                if (packet_id == 0x20) _ = c.cs_setPlayerMovementFlags(ctx, client_fd, &on_ground);
                if (packet_id == 0x1E) _ = c.cs_setPlayerPositionAndRotation(ctx, client_fd, &x, &y, &z, &yaw, &pitch, &on_ground);

                var player_ptr: ?*c.PlayerData = null;
                if (c.getPlayerData(ctx, client_fd, &player_ptr) != 0) break :sw;
                const player = player_ptr.?;

                const block_feet = c.getBlockAt(ctx, @intCast(player.x), @intCast(player.y), @intCast(player.z));
                const swimming = block_feet >= c.B_water and block_feet < c.B_water + 8;

                if (on_ground != 0) {
                    const damage: i16 = @intCast(@as(i32, player.grounded_y) - @as(i32, @intCast(player.y)) - 3);
                    if (damage > 0 and (c.GAMEMODE == 0 or c.GAMEMODE == 2) and !swimming) {
                        c.hurtEntity(ctx, client_fd, -1, c.D_fall, @intCast(damage));
                    }
                    player.grounded_y = player.y;
                } else if (swimming) {
                    player.grounded_y = player.y;
                }

                if (packet_id == 0x20) break :sw;

                if (packet_id != 0x1D) {
                    const yaw_i: i16 = @intFromFloat(yaw + 540);
                    const yaw_wrapped: i16 = @mod(yaw_i, 360);
                    const yaw_centered: i16 = yaw_wrapped - 180;
                    player.yaw = @intCast(@divTrunc(yaw_centered * 127, 180));
                    player.pitch = @intFromFloat(@as(f32, pitch) / 90.0 * 127.0);
                }

                var should_broadcast: bool = true;
                if (comptime !@hasDecl(c, "BROADCAST_ALL_MOVEMENT")) {
                    should_broadcast = (player.flags & 0x40) == 0;
                    if (should_broadcast) player.flags |= 0x40;
                }
                if (comptime @hasDecl(c, "SCALE_MOVEMENT_UPDATES_TO_PLAYER_COUNT")) {
                    player.packets_since_update += 1;
                    if (player.packets_since_update < ctx.client_count) {
                        should_broadcast = false;
                    } else {
                        player.packets_since_update = 0;
                    }
                }

                if (should_broadcast) {
                    var yaw_loc = yaw;
                    var pitch_loc = pitch;
                    if (packet_id == 0x1D) {
                        yaw_loc = @as(f32, @floatFromInt(player.yaw)) * 180.0 / 127.0;
                        pitch_loc = @as(f32, @floatFromInt(player.pitch)) * 90.0 / 127.0;
                    }
                    for (0..c.MAX_PLAYERS) |i| {
                        const other_player = ctx.player_data[i];
                        if (other_player.client_fd == -1 or (other_player.flags & 0x20 != 0) or other_player.client_fd == client_fd) continue;
                        if (packet_id == 0x1F) {
                            _ = c.sc_updateEntityRotation(
                                other_player.client_fd,
                                client_fd,
                                std.math.lossyCast(u8, player.yaw),
                                std.math.lossyCast(u8, player.pitch),
                            );
                        } else {
                            _ = c.sc_teleportEntity(other_player.client_fd, client_fd, x, y, z, yaw_loc, pitch_loc);
                        }
                        _ = c.sc_setHeadRotation(other_player.client_fd, client_fd, std.math.lossyCast(u8, player.yaw));
                    }
                }

                if (packet_id == 0x1F) break :sw;

                if (player.saturation == 0) {
                    if (player.hunger > 0) player.hunger -= 1;
                    player.saturation = 200;
                    _ = c.sc_setHealth(client_fd, player.health, player.hunger, player.saturation);
                } else if ((player.flags & 0x08) != 0) {
                    player.saturation -= 1;
                }

                var cx: i16 = @intFromFloat(x);
                var cy: i16 = @intFromFloat(y);
                var cz: i16 = @intFromFloat(z);
                if (x < 0) cx -= 1;
                if (z < 0) cz -= 1;

                const _x: i16 = if (cx < 0) @divTrunc(cx - 16, 16) else @divTrunc(cx, 16);
                const _z: i16 = if (cz < 0) @divTrunc(cz - 16, 16) else @divTrunc(cz, 16);
                const old_player_x: i16 = if (player.x < 0) @divTrunc(player.x - 16, 16) else @divTrunc(player.x, 16);
                const old_player_z: i16 = if (player.z < 0) @divTrunc(player.z - 16, 16) else @divTrunc(player.z, 16);

                var dx: i16 = _x - old_player_x;
                var dz: i16 = _z - old_player_z;

                if (cy < 0) {
                    cy = 0;
                    player.grounded_y = 0;
                    _ = c.sc_synchronizePlayerPosition(client_fd, @as(f64, @floatFromInt(cx)), 0, @as(f64, @floatFromInt(cz)), @as(f32, @floatFromInt(player.yaw)) * 180.0 / 127.0, @as(f32, @floatFromInt(player.pitch)) * 90.0 / 127.0);
                } else if (cy > 255) {
                    cy = 255;
                    _ = c.sc_synchronizePlayerPosition(client_fd, @as(f64, @floatFromInt(cx)), 255, @as(f64, @floatFromInt(cz)), @as(f32, @floatFromInt(player.yaw)) * 180.0 / 127.0, @as(f32, @floatFromInt(player.pitch)) * 90.0 / 127.0);
                }
                player.x = cx;
                player.y = @intCast(cy);
                player.z = cz;

                if (dx == 0 and dz == 0) break :sw;

                var found = false;
                for (0..c.VISITED_HISTORY) |i| {
                    if (player.visited_x[i] == _x and player.visited_z[i] == _z) {
                        found = true;
                        break;
                    }
                }
                if (found) break :sw;

                for (0..c.VISITED_HISTORY - 1) |i| {
                    player.visited_x[i] = player.visited_x[i + 1];
                    player.visited_z[i] = player.visited_z[i + 1];
                }
                player.visited_x[c.VISITED_HISTORY - 1] = _x;
                player.visited_z[c.VISITED_HISTORY - 1] = _z;

                const r_mob = c.fast_rand(ctx);
                if ((r_mob & 3) == 0) {
                    const mob_x: i16 = @intCast(@as(c_int, _x + dx * c.VIEW_DISTANCE) * 16 + @as(c_int, @intCast((r_mob >> 4) & 15)));
                    const mob_z: i16 = @intCast(@as(c_int, _z + dz * c.VIEW_DISTANCE) * 16 + @as(c_int, @intCast((r_mob >> 8) & 15)));
                    var mob_y: u8 = @intCast(cy - 8);
                    var b_low = c.getBlockAt(ctx, mob_x, mob_y - 1, mob_z);
                    var b_mid = c.getBlockAt(ctx, mob_x, mob_y, mob_z);
                    var b_top = c.getBlockAt(ctx, mob_x, mob_y + 1, mob_z);
                    while (mob_y < 255) {
                        if (c.isPassableBlock(b_low) == 0 and c.isPassableSpawnBlock(b_mid) != 0 and c.isPassableSpawnBlock(b_top) != 0) break;
                        b_low = b_mid;
                        b_mid = b_top;
                        b_top = c.getBlockAt(ctx, mob_x, @as(c_int, mob_y) + 2, mob_z);
                        mob_y += 1;
                    }
                    if (mob_y != 255) {
                        if ((ctx.world_time < 13000 or ctx.world_time > 23460) and mob_y > 48) {
                            const mob_choice = (r_mob >> 12) & 3;
                            if (mob_choice == 0) c.spawnMob(ctx, 25, mob_x, mob_y, mob_z, 4) else if (mob_choice == 1) c.spawnMob(ctx, 28, mob_x, mob_y, mob_z, 10) else if (mob_choice == 2) c.spawnMob(ctx, 95, mob_x, mob_y, mob_z, 10) else if (mob_choice == 3) c.spawnMob(ctx, 106, mob_x, mob_y, mob_z, 8);
                        } else {
                            c.spawnMob(ctx, 145, mob_x, mob_y, mob_z, 20);
                        }
                    }
                }

                _ = c.sc_setCenterChunk(client_fd, _x, _z);
                while (dx != 0) {
                    _ = c.sc_chunkDataAndUpdateLight(ctx, client_fd, _x + dx * c.VIEW_DISTANCE, _z);
                    var i: c_int = 1;
                    while (i <= c.VIEW_DISTANCE) : (i += 1) {
                        _ = c.sc_chunkDataAndUpdateLight(ctx, client_fd, _x + dx * c.VIEW_DISTANCE, _z - i);
                        _ = c.sc_chunkDataAndUpdateLight(ctx, client_fd, _x + dx * c.VIEW_DISTANCE, _z + i);
                    }
                    dx += if (dx > 0) -1 else 1;
                }
                while (dz != 0) {
                    _ = c.sc_chunkDataAndUpdateLight(ctx, client_fd, _x, _z + dz * c.VIEW_DISTANCE);
                    var i: c_int = 1;
                    while (i <= c.VIEW_DISTANCE) : (i += 1) {
                        _ = c.sc_chunkDataAndUpdateLight(ctx, client_fd, _x - i, _z + dz * c.VIEW_DISTANCE);
                        _ = c.sc_chunkDataAndUpdateLight(ctx, client_fd, _x + i, _z + dz * c.VIEW_DISTANCE);
                    }
                    dz += if (dz > 0) -1 else 1;
                }
            }
        },
        0x29 => {
            if (c.getClientState(ctx, client_fd) == c.STATE_PLAY) _ = c.cs_playerCommand(ctx, client_fd);
        },
        0x2A => {
            if (c.getClientState(ctx, client_fd) == c.STATE_PLAY) _ = c.cs_playerInput(ctx, client_fd);
        },
        0x2B => {
            if (c.getClientState(ctx, client_fd) == c.STATE_PLAY) _ = c.cs_playerLoaded(ctx, client_fd);
        },
        0x34 => {
            if (c.getClientState(ctx, client_fd) == c.STATE_PLAY) _ = c.cs_setHeldItem(ctx, client_fd);
        },
        0x3C => {
            if (c.getClientState(ctx, client_fd) == c.STATE_PLAY) _ = c.cs_swingArm(ctx, client_fd);
        },
        0x28 => {
            if (c.getClientState(ctx, client_fd) == c.STATE_PLAY) _ = c.cs_playerAction(ctx, client_fd);
        },
        0x3F => {
            if (c.getClientState(ctx, client_fd) == c.STATE_PLAY) _ = c.cs_useItemOn(ctx, client_fd);
        },
        0x40 => {
            if (c.getClientState(ctx, client_fd) == c.STATE_PLAY) _ = c.cs_useItem(ctx, client_fd);
        },
        else => {
            if (comptime @hasDecl(c, "DEV_LOG_UNKNOWN_PACKETS")) {
                std.log.warn("Unknown packet: 0x{X:0>2}, length: {d}, state: {d}", .{ packet_id, length, c.getClientState(ctx, client_fd) });
            }
            _ = c.recv_all(client_fd, &ctx.recv_buffer, @intCast(length), 0);
        },
    }

    const processed_length: c_int = @intCast(tools.total_bytes_received - bytes_received_start);
    if (processed_length == length) return;
    if (length > processed_length) {
        _ = c.recv_all(client_fd, &ctx.recv_buffer, @intCast(length - processed_length), 0);
    }

    if (comptime @hasDecl(c, "DEV_LOG_LENGTH_DISCREPANCY")) {
        if (processed_length != 0) {
            std.log.warn("WARNING: Packet 0x{X:0>2} parsed incorrectly!", .{packet_id});
            std.log.warn("  Expected: {d}, parsed: {d}", .{ length, processed_length });
        }
    }
    if (comptime @hasDecl(c, "DEV_LOG_UNKNOWN_PACKETS")) {
        if (processed_length == 0) {
            std.log.warn("Unknown packet: 0x{X:0>2}, length: {d}, state: {d}", .{ packet_id, length, c.getClientState(ctx, client_fd) });
        }
    }
}
