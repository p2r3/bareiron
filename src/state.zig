const std = @import("std");
const c = @import("c_api.zig").c;

// Keep layout compatible with C's ServerContext in include/context.h
pub const ServerContext = extern struct {
    world_seed: u32,
    rng_seed: u32,
    world_time: u16,
    server_ticks: u32,
    client_count: u16,

    block_changes: [c.MAX_BLOCK_CHANGES]c.BlockChange,
    block_changes_count: c_int,

    player_data: [c.MAX_PLAYERS]c.PlayerData,
    player_data_count: c_int,

    mob_data: [c.MAX_MOBS]c.MobData,

    motd_len: u8,
    motd: [64]u8,
    // brand gated by SEND_BRAND in C; we still reserve space for simplicity.
    brand_len: u8,
    brand: [32]u8,
};

pub const ServerState = struct {
    context: ServerContext,

    pub fn init() ServerState {
        var s: ServerState = undefined;
    // Zero everything by default
    @memset(std.mem.asBytes(&s.context), 0);

        s.context.world_seed = c.INITIAL_WORLD_SEED;
        s.context.rng_seed = c.INITIAL_RNG_SEED;
        s.context.world_time = 0;
        s.context.server_ticks = 0;
        s.context.client_count = 0;
        s.context.block_changes_count = 0;
        s.context.player_data_count = 0;

        // default strings
        const default_motd = "A bareiron server";
        s.context.motd_len = @intCast(default_motd.len);
    std.mem.copyForwards(u8, s.context.motd[0..default_motd.len], default_motd);
        const default_brand = "bareiron";
        s.context.brand_len = @intCast(default_brand.len);
    std.mem.copyForwards(u8, s.context.brand[0..default_brand.len], default_brand);

        // mark players as disconnected
        var i: usize = 0;
        while (i < s.context.player_data.len) : (i += 1) {
            s.context.player_data[i].client_fd = -1;
        }
        // mark block change gaps
        var j: usize = 0;
        while (j < s.context.block_changes.len) : (j += 1) {
            s.context.block_changes[j].block = 0xFF;
        }
        return s;
    }
};
