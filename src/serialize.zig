const std = @import("std");
const c = @import("c_api.zig").c;

const HAS_SYNC: bool = @hasDecl(c, "SYNC_WORLD_TO_DISK");
const HAS_ESP: bool = @hasDecl(c, "ESP_PLATFORM");
const FILE_PATH = if (HAS_ESP) "/littlefs/world.bin" else "world.bin";

var last_disk_sync_time: i64 = 0;

fn openRwFile() !std.fs.File {
    return std.fs.cwd().openFile(FILE_PATH, .{ .mode = .read_write });
}

fn openReadFile() !std.fs.File {
    return std.fs.cwd().openFile(FILE_PATH, .{});
}

fn createWorldFile(ctx: *c.ServerContext) c_int {
    const cwd = std.fs.cwd();
    const file = cwd.createFile(FILE_PATH, .{}) catch |e| {
        std.log.err("Failed to create '{s}': {s}", .{ FILE_PATH, @errorName(e) });
        return 1;
    };
    defer file.close();

    file.writeAll(std.mem.asBytes(&ctx.block_changes)) catch |e| {
        std.log.err("Failed to write initial block data to '{s}': {s}", .{ FILE_PATH, @errorName(e) });
        return 1;
    };
    file.writeAll(std.mem.asBytes(&ctx.player_data)) catch |e| {
        std.log.err("Failed to write initial player data to '{s}': {s}", .{ FILE_PATH, @errorName(e) });
        return 1;
    };
    return 0;
}

// Restores world data from disk, or writes world file if it doesn't exist
pub export fn initSerializer(ctx: *c.ServerContext) c_int {
    if (!HAS_SYNC) {
        return 0;
    }

    last_disk_sync_time = c.get_program_time();

    // Try open existing file for reading
    const file = openReadFile() catch |err| switch (err) {
        error.FileNotFound => return createWorldFile(ctx),
        else => {
            std.log.err("Failed to open '{s}' for read: {s}", .{ FILE_PATH, @errorName(err) });
            return 1;
        },
    };
    defer file.close();

    // Read full block_changes
    const bc_bytes = std.mem.asBytes(&ctx.block_changes);
    const read_bc = file.readAll(bc_bytes) catch |e| {
        std.log.err("Failed to read block changes from '{s}': {s}", .{ FILE_PATH, @errorName(e) });
        return 1;
    };
    if (read_bc != bc_bytes.len) {
        std.log.err(
            "Read {} bytes from '{s}', expected {} (block changes). Aborting.",
            .{ read_bc, FILE_PATH, bc_bytes.len },
        );
        return 1;
    }

    // Recompute block_changes_count as in C
    var i: c_int = 0;
    while (i < c.MAX_BLOCK_CHANGES) : (i += 1) {
        const idx: usize = @intCast(i);
        const blk = ctx.block_changes[idx].block;
        if (blk == 0xFF) continue;
        if (blk == c.B_chest) i += 14; // matches C's i += 14 inside loop
        if (i >= ctx.block_changes_count) ctx.block_changes_count = i + 1;
    }

    // Seek to player data start and read it
    file.seekTo(@sizeOf(@TypeOf(ctx.block_changes))) catch |e| {
        std.log.err("Failed to seek to player data in '{s}': {s}", .{ FILE_PATH, @errorName(e) });
        return 1;
    };
    const pd_bytes = std.mem.asBytes(&ctx.player_data);
    const read_pd = file.readAll(pd_bytes) catch |e| {
        std.log.err("Failed to read player data from '{s}': {s}", .{ FILE_PATH, @errorName(e) });
        return 1;
    };
    if (read_pd != pd_bytes.len) {
        std.log.err(
            "Read {} bytes from '{s}', expected {} (player data). Aborting.",
            .{ read_pd, FILE_PATH, pd_bytes.len },
        );
        return 1;
    }

    return 0;
}

// Writes a range of block change entries to disk (inclusive range)
pub export fn writeBlockChangesToDisk(ctx: *c.ServerContext, from: c_int, to: c_int) void {
    if (!HAS_SYNC) {
        return;
    }
    const file = openRwFile() catch |err| {
        std.log.warn("Failed to open '{s}'. Block updates have been dropped: {s}", .{ FILE_PATH, @errorName(err) });
        return;
    };
    defer file.close();

    var i = from;
    while (i <= to) : (i += 1) {
        const off: u64 = @intCast(@as(usize, @intCast(i)) * @sizeOf(c.BlockChange));
        file.seekTo(off) catch |e| {
            std.log.warn("Failed to seek in '{s}': {s}. Block updates dropped.", .{ FILE_PATH, @errorName(e) });
            return;
        };
        const idx: usize = @intCast(i);
        const bytes = std.mem.asBytes(&ctx.block_changes[idx]);
        file.writeAll(bytes) catch |e| {
            std.log.warn("Failed to write to '{s}': {s}. Block updates dropped.", .{ FILE_PATH, @errorName(e) });
            return;
        };
    }
}

// Writes all player data to disk
pub export fn writePlayerDataToDisk(ctx: *c.ServerContext) void {
    if (!HAS_SYNC) {
        return;
    }
    const file = openRwFile() catch |err| {
        std.log.warn("Failed to open '{s}'. Player updates have been dropped: {s}", .{ FILE_PATH, @errorName(err) });
        return;
    };
    defer file.close();

    file.seekTo(@sizeOf(@TypeOf(ctx.block_changes))) catch |e| {
        std.log.warn("Failed to seek in '{s}': {s}. Player updates dropped.", .{ FILE_PATH, @errorName(e) });
        return;
    };
    file.writeAll(std.mem.asBytes(&ctx.player_data)) catch |e| {
        std.log.warn("Failed to write to '{s}': {s}. Player updates dropped.", .{ FILE_PATH, @errorName(e) });
        return;
    };
}

// Writes data queued for interval writes, but only if enough time has passed
pub export fn writeDataToDiskOnInterval(ctx: *c.ServerContext) void {
    if (!HAS_SYNC) {
        return;
    }
    if (c.get_program_time() - last_disk_sync_time < c.DISK_SYNC_INTERVAL) return;
    last_disk_sync_time = c.get_program_time();
    writePlayerDataToDisk(ctx);
    if (@hasDecl(c, "DISK_SYNC_BLOCKS_ON_INTERVAL")) {
        writeBlockChangesToDisk(ctx, 0, ctx.block_changes_count);
    }
}

// Writes a chest slot change to disk (only when ALLOW_CHESTS)
pub export fn writeChestChangesToDisk(ctx: *c.ServerContext, storage_ptr: [*]u8, slot: u8) void {
    if (!HAS_SYNC) {
        return;
    }
    if (!@hasDecl(c, "ALLOW_CHESTS")) return;
    const base_addr: usize = @intFromPtr(&ctx.block_changes);
    const stor_addr: usize = @intFromPtr(storage_ptr);
    const byte_offset: usize = stor_addr - base_addr;
    const index: c_int = @intCast(byte_offset / @sizeOf(c.BlockChange) + (slot / 2));
    writeBlockChangesToDisk(ctx, index, index);
}
