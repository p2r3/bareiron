const std = @import("std");
const c = @import("c_api.zig").c;
comptime {
    _ = @import("varnum.zig");
}
comptime {
    _ = @import("structures.zig");
}
comptime {
    _ = @import("crafting.zig");
}
const state_mod = @import("state.zig");
const builtin = @import("builtin");

const MAX_PLAYERS = c.MAX_PLAYERS;
const PORT: u16 = @intCast(c.PORT);
const TIME_BETWEEN_TICKS: i64 = @intCast(c.TIME_BETWEEN_TICKS);

const is_windows = builtin.target.os.tag == .windows;
const SocketFD = if (is_windows) c.SOCKET else std.posix.fd_t;
const INVALID_FD: SocketFD = if (is_windows) c.INVALID_SOCKET else @as(std.posix.fd_t, -1);
var client_fds: [MAX_PLAYERS]SocketFD = undefined;
var g_state: state_mod.ServerState = undefined;

const is_esp = @import("builtin").target.os.tag == .freestanding;

// Yield helper for platforms (ESP). Implemented in Zig to avoid a separate C file.
var last_yield: i64 = 0;
fn task_yield() void {
    if (!is_esp) return;
    // TASK_YIELD_INTERVAL = 1000 * 1000 (microseconds)
    const TASK_YIELD_INTERVAL: i64 = 1000 * 1000;
    const TASK_YIELD_TICKS: c_int = 1;
    const time_now = c.esp_timer_get_time();
    if (time_now - last_yield < TASK_YIELD_INTERVAL) return;
    _ = c.vTaskDelay(TASK_YIELD_TICKS);
    last_yield = time_now;
}

pub fn main() !void {
    if (is_windows) {
        var wsa_data: c.WSADATA = undefined;
        if (c.WSAStartup(c.MAKEWORD(2, 2), &wsa_data) != 0) {
            std.log.err("WSAStartup failed", .{});
            return error.WinsockInitFailed;
        }
        defer _ = c.WSACleanup();
    }

    // Initialize server state context
    g_state = state_mod.ServerState.init();
    _ = c.initSerializer(@ptrCast(&g_state.context));

    for (&client_fds) |*fd| fd.* = INVALID_FD;

    var server_fd: SocketFD = undefined;
    // Create TCP server socket (IPv4)
    server_fd = c.socket(c.AF_INET, c.SOCK_STREAM, 0);
    if (server_fd == INVALID_FD) return error.SocketCreateFailed;
    defer {
        if (is_windows) {
            _ = c.closesocket(server_fd);
        } else {
            _ = c.close(server_fd);
        }
    }

    var opt: c_int = 1;
    _ = c.setsockopt(server_fd, c.SOL_SOCKET, c.SO_REUSEADDR, @ptrCast(&opt), @sizeOf(c_int));

    var addr: c.sockaddr_in = std.mem.zeroes(c.sockaddr_in);
    addr.sin_family = c.AF_INET;
    addr.sin_port = c.htons(PORT);
    addr.sin_addr.s_addr = c.htonl(c.INADDR_ANY);
    if (c.bind(server_fd, @ptrCast(&addr), @intCast(@sizeOf(c.sockaddr_in))) < 0) {
        std.log.err("bind() failed", .{});
        return error.BindFailed;
    }
    if (c.listen(server_fd, 5) < 0) {
        std.log.err("listen() failed", .{});
        return error.ListenFailed;
    }
    if (is_windows) {
        var mode: c_ulong = 1;
        _ = c.ioctlsocket(server_fd, c.FIONBIO, &mode);
    } else {
        const flags: c_int = c.fcntl(server_fd, c.F_GETFL, @as(c_int, 0));
        _ = c.fcntl(server_fd, c.F_SETFL, flags | @as(c_int, c.O_NONBLOCK));
    }
    std.log.info("Server listening on port {}", .{PORT});

    var last_tick_time: i64 = c.get_program_time();

    while (true) {
        const now: i64 = c.get_program_time();
        const elapsed = now - last_tick_time;
        var time_to_next_tick: i64 = TIME_BETWEEN_TICKS - elapsed;
        if (time_to_next_tick < 0) time_to_next_tick = 0;

        if (is_windows) {
            var read_fds: c.fd_set = undefined;
            c.FD_ZERO(&read_fds);
            c.FD_SET(server_fd, &read_fds);

            var max_fd: SocketFD = server_fd;
            for (client_fds) |fd| {
                if (fd != INVALID_FD) {
                    c.FD_SET(fd, &read_fds);
                    if (fd > max_fd) max_fd = fd;
                }
            }

            var timeout: c.timeval = .{
                .tv_sec = @intCast(time_to_next_tick / 1_000_000),
                .tv_usec = @intCast(time_to_next_tick % 1_000_000),
            };

            const activity = c.select(@intCast(max_fd + 1), &read_fds, null, null, &timeout);
            if (activity < 0) {
                std.log.warn("select() returned an error, continuing.", .{});
            }

            if (c.FD_ISSET(server_fd, &read_fds)) {
                acceptNewConnection(server_fd) catch |err| {
                    std.log.warn("Failed to accept new connection: {s}", .{@errorName(err)});
                };
            }

            for (0..MAX_PLAYERS) |i| {
                const fd = client_fds[i];
                if (fd != INVALID_FD and c.FD_ISSET(fd, &read_fds)) {
                    var peek_buf: [1]u8 = undefined;
                    const peek_res = c.recv(fd, &peek_buf, 1, c.MSG_PEEK);
                    if (peek_res > 0) {
                        processClientPacket(fd);
                    } else if (peek_res == 0) {
                        // orderly shutdown by peer
                        disconnectClient(i);
                    } else {
                        // peek_res < 0: check errno for non-blocking 'would block'
                        const err = c.get_errno();
                        if (err == c.EAGAIN or err == c.EWOULDBLOCK) {
                            // no data available yet, continue
                        } else {
                            disconnectClient(i);
                        }
                    }
                }
            }
        } else {
            var pfds: [MAX_PLAYERS + 1]c.pollfd = undefined;
            var idxmap: [MAX_PLAYERS + 1]isize = undefined;
            var count: usize = 0;
            pfds[count] = .{ .fd = server_fd, .events = c.POLLIN, .revents = 0 };
            idxmap[count] = -1; // server marker
            count += 1;

            for (0..MAX_PLAYERS) |i| {
                const fd = client_fds[i];
                if (fd == INVALID_FD) continue;
                pfds[count] = .{ .fd = fd, .events = c.POLLIN, .revents = 0 };
                idxmap[count] = @intCast(i);
                count += 1;
            }

            const timeout_ms: i32 = @intCast(@divTrunc(time_to_next_tick, 1000));
            const rc = c.poll(&pfds[0], @intCast(count), timeout_ms);
            if (rc < 0) {
                std.log.warn("poll() returned an error, continuing.", .{});
            }

            if (pfds[0].revents & c.POLLIN != 0) {
                acceptNewConnection(server_fd) catch |err| {
                    std.log.warn("Failed to accept new connection: {s}", .{@errorName(err)});
                };
            }

            var i: usize = 1;
            while (i < count) : (i += 1) {
                const ev = pfds[i].revents;
                if (ev == 0) continue;
                const slot = idxmap[i];
                if (slot < 0) continue;
                const fd = client_fds[@intCast(slot)];
                if (ev & (c.POLLHUP | c.POLLERR | c.POLLNVAL) != 0) {
                    disconnectClient(@intCast(slot));
                    continue;
                }
                if (ev & c.POLLIN != 0) {
                    var peek_buf: [1]u8 = undefined;
                    const peek_res = c.recv(fd, &peek_buf, 1, c.MSG_PEEK);
                    if (peek_res > 0) {
                        processClientPacket(fd);
                    } else if (peek_res == 0) {
                        // orderly shutdown by peer
                        disconnectClient(@intCast(slot));
                    } else {
                        // peek_res < 0: on non-blocking sockets EAGAIN/EWOULDBLOCK is normal
                        const err = c.get_errno();
                        if (err == c.EAGAIN or err == c.EWOULDBLOCK) {
                            // no data available; do nothing
                        } else {
                            disconnectClient(@intCast(slot));
                        }
                    }
                }
            }
        }

        const after_wait: i64 = c.get_program_time();
        if ((after_wait - last_tick_time) >= TIME_BETWEEN_TICKS) {
            c.handleServerTick(@ptrCast(&g_state.context), after_wait - last_tick_time);
            last_tick_time = after_wait;
        }
    }
}

fn acceptNewConnection(server_fd: SocketFD) !void {
    var free_slot: ?usize = null;
    for (0..MAX_PLAYERS) |i| {
        if (client_fds[i] == INVALID_FD) {
            free_slot = i;
            break;
        }
    }
    var addr: c.sockaddr_in = undefined;
    var addr_len: c.socklen_t = @intCast(@sizeOf(c.sockaddr_in));
    const new_fd = c.accept(server_fd, @ptrCast(&addr), &addr_len);
    if (new_fd == INVALID_FD) return error.AcceptFailed;

    if (free_slot) |i| {
        if (is_windows) {
            var mode: c_ulong = 1;
            _ = c.ioctlsocket(new_fd, c.FIONBIO, &mode);
        } else {
            const flags2: c_int = c.fcntl(new_fd, c.F_GETFL, @as(c_int, 0));
            _ = c.fcntl(new_fd, c.F_SETFL, flags2 | @as(c_int, c.O_NONBLOCK));
        }
        client_fds[i] = new_fd;
        g_state.context.client_count += 1;
        std.log.info("Accepted new client in slot {d} (fd: {d})", .{ i, new_fd });
        c.setClientState(@ptrCast(&g_state.context), @intCast(new_fd), c.STATE_NONE);
    } else {
        if (is_windows) {
            _ = c.closesocket(new_fd);
        } else {
            _ = c.close(new_fd);
        }
    }
}

fn disconnectClient(slot: usize) void {
    const fd = client_fds[slot];
    if (fd == INVALID_FD) return;
    std.log.info("Client in slot {d} (fd: {d}) disconnected.", .{ slot, fd });
    c.setClientState(@ptrCast(&g_state.context), @intCast(fd), c.STATE_NONE);
    c.handlePlayerDisconnect(@ptrCast(&g_state.context), @intCast(fd));
    if (is_windows) {
        _ = c.closesocket(fd);
    } else {
        _ = c.close(fd);
    }
    client_fds[slot] = INVALID_FD;
    if (g_state.context.client_count > 0) g_state.context.client_count -= 1;
}

fn processClientPacket(fd: SocketFD) void {
    const length = c.readVarInt(@ptrCast(&g_state.context), @intCast(fd));
    if (length == c.VARNUM_ERROR) return;
    const packet_id = c.readVarInt(@ptrCast(&g_state.context), @intCast(fd));
    if (packet_id == c.VARNUM_ERROR) return;
    const st = c.getClientState(@ptrCast(&g_state.context), @intCast(fd));
    c.handlePacket(@ptrCast(&g_state.context), @intCast(fd), length - c.sizeVarInt(@intCast(packet_id)), @intCast(packet_id), st);
}
