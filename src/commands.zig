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

    if (std.mem.startsWith(u8, command, "give shears")) {
        _ = c.givePlayerItem(player, c.I_shears, 1);
        const msg = "Gave 1 x Shears.";
        _ = c.sc_systemChat(client_fd, @ptrCast(@constCast(msg.ptr)), @intCast(msg.len));
        return;
    }

    if (std.mem.startsWith(u8, command, "spawn sheep")) {
        c.spawnMob(ctx, 106, player.x, player.y, player.z + 2, 8);
        const msg = "Spawned a sheep.";
        _ = c.sc_systemChat(client_fd, @ptrCast(@constCast(msg.ptr)), @intCast(msg.len));
        return;
    }

    const unknown = "Unknown command.";
    _ = c.sc_systemChat(client_fd, @ptrCast(@constCast(unknown.ptr)), @intCast(unknown.len));
}
