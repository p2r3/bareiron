const std = @import("std");
const c = @import("c_api.zig").c;

pub export fn handleChatCommand(ctx: *c.ServerContext, client_fd: c_int) void {
    var player_ptr: [*c]c.PlayerData = undefined;
    if (c.getPlayerData(ctx, client_fd, &player_ptr) != 0) return;
    if (player_ptr == null) return;
    const player: *c.PlayerData = @ptrCast(player_ptr);

    const buf: []const u8 = ctx.recv_buffer[0..];
    const end: usize = std.mem.indexOfScalar(u8, buf, 0) orelse buf.len;
    const command = buf[0..end];

    // Simple tokenizer: split by spaces (up to 3 tokens)
    var it = std.mem.tokenizeScalar(u8, command, ' ');
    const cmd = it.next() orelse return;

    if (std.mem.eql(u8, cmd, "give")) {
        const item_name_raw = it.next() orelse {
            const msg = "Usage: /give <item> [count]";
            _ = c.sc_systemChat(client_fd, @ptrCast(@constCast(msg.ptr)), @intCast(msg.len));
            return;
        };
        const count_str = it.next();
        var count: u8 = 1;
        if (count_str) |cs| {
            const parsed = std.fmt.parseInt(u8, cs, 10) catch 1;
            count = if (parsed == 0) 1 else parsed;
        }
        const item_id_opt = lookupId(item_name_raw, c.item_names, c.item_ids, c.ITEM_NAME_COUNT);
        if (item_id_opt) |item_id| {
            _ = c.givePlayerItem(player, item_id, count);
            var outbuf: [96]u8 = undefined;
            const msg = std.fmt.bufPrint(&outbuf, "Gave {d} x {s}.", .{ count, item_name_raw }) catch return;
            _ = c.sc_systemChat(client_fd, @ptrCast(msg.ptr), @intCast(msg.len));
        } else {
            var outbuf: [96]u8 = undefined;
            const msg = std.fmt.bufPrint(&outbuf, "Unknown item: {s}", .{item_name_raw}) catch return;
            _ = c.sc_systemChat(client_fd, @ptrCast(msg.ptr), @intCast(msg.len));
        }
        return;
    }

    if (std.mem.eql(u8, cmd, "spawn")) {
        const ent_name_raw = it.next() orelse {
            const msg = "Usage: /spawn <entity>";
            _ = c.sc_systemChat(client_fd, @ptrCast(@constCast(msg.ptr)), @intCast(msg.len));
            return;
        };
        const ent_id_opt = lookupId(ent_name_raw, c.entity_type_names, c.entity_type_ids, c.ENTITY_TYPE_COUNT);
        if (ent_id_opt) |ent_id| {
            c.spawnMob(ctx, @intCast(ent_id), player.x, player.y, player.z + 2, 8);
            var outbuf: [96]u8 = undefined;
            const msg = std.fmt.bufPrint(&outbuf, "Spawned {s}.", .{ent_name_raw}) catch return;
            _ = c.sc_systemChat(client_fd, @ptrCast(msg.ptr), @intCast(msg.len));
        } else {
            var outbuf: [96]u8 = undefined;
            const msg = std.fmt.bufPrint(&outbuf, "Unknown entity: {s}", .{ent_name_raw}) catch return;
            _ = c.sc_systemChat(client_fd, @ptrCast(msg.ptr), @intCast(msg.len));
        }
        return;
    }

    const unknown = "Unknown command.";
    _ = c.sc_systemChat(client_fd, @ptrCast(@constCast(unknown.ptr)), @intCast(unknown.len));
}

fn lookupId(name: []const u8, names: [*c][*c]const u8, ids: [*c]u16, count: c_int) ?u16 {
    // Normalize: accept names with/without namespace; convert spaces to underscores
    var tmp_buf: [128]u8 = undefined;
    const normalized = normalizeName(&tmp_buf, name);
    var i: usize = 0;
    const total: usize = @as(usize, @intCast(count));
    while (i < total) : (i += 1) {
        const cstr: [*c]const u8 = names[i];
        if (cstr == null) continue;
        const entry = std.mem.sliceTo(cstr, 0);
        if (std.mem.eql(u8, entry, normalized)) {
            return ids[i];
        }
    }
    return null;
}

fn normalizeName(buf: *[128]u8, name: []const u8) []const u8 {
    var w: usize = 0;
    var i: usize = 0;
    // Strip optional namespace prefix "minecraft:"
    var start: usize = 0;
    if (std.mem.indexOfScalar(u8, name, ':')) |colon| {
        start = colon + 1;
    }
    while (i + start < name.len and w < buf.len - 1) : (i += 1) {
        const ch = name[i + start];
        buf[w] = if (ch == ' ') '_' else std.ascii.toLower(ch);
        w += 1;
    }
    buf[w] = 0;
    return buf[0..w];
}
