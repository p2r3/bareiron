const std = @import("std");
const builtin = @import("builtin");
const c = @import("c_api.zig").c;

pub export var total_bytes_received: u64 = 0;

const is_esp = builtin.target.os.tag == .freestanding;
const is_windows = builtin.target.os.tag == .windows;

var last_yield_time: i64 = 0;
fn task_yield() void {
    if (comptime is_esp) {
        const TASK_YIELD_INTERVAL: i64 = 1_000_000; // microseconds
        const TASK_YIELD_TICKS: c.BaseType_t = 1;
        const now = c.esp_timer_get_time();
        if (now - last_yield_time >= TASK_YIELD_INTERVAL) {
            _ = c.vTaskDelay(TASK_YIELD_TICKS);
            last_yield_time = now;
        }
    } else {
        // no-op on non-ESP
    }
}

extern "c" fn __errno_location() *c_int; // glibc/Linux
extern "c" fn _errno() *c_int; // Windows/MSVCRT

fn errnoPtr() *c_int {
    if (is_windows) {
        return _errno();
    } else {
        return __errno_location();
    }
}

inline fn set_errno(val: c_int) void {
    errnoPtr().* = val;
}

pub export fn get_errno() c_int {
    return errnoPtr().*;
}

inline fn wouldBlockSend() bool {
    if (comptime is_windows) {
        const err = c.WSAGetLastError();
        return err == c.WSAEWOULDBLOCK or err == c.WSAEINTR;
    } else {
        const e = errnoPtr().*;
        return e == c.EINTR or e == c.EAGAIN or e == c.EWOULDBLOCK;
    }
}

// Monotonic time in microseconds
pub export fn get_program_time() i64 {
    if (comptime is_esp) {
        return c.esp_timer_get_time();
    } else {
        const ns: i128 = std.time.nanoTimestamp();
        return @intCast(@divTrunc(ns, 1000));
    }
}

pub export fn recv_all(client_fd: c_int, buf: ?*anyopaque, n: usize, require_first: u8) isize {
    if (buf == null) return -1;
    const p: [*]u8 = @ptrCast(buf.?);
    var total: usize = 0;
    var last_update_time = get_program_time();

    if (require_first != 0) {
        const r = c.recv(client_fd, p, 1, c.MSG_PEEK);
        if (r <= 0) {
            if (r < 0 and (get_errno() == c.EAGAIN or get_errno() == c.EWOULDBLOCK)) {
                return 0;
            }
            return -1;
        }
    }

    while (total < n) {
        const r = c.recv(client_fd, p + total, n - total, 0);
        if (r < 0) {
            if (get_errno() == c.EAGAIN or get_errno() == c.EWOULDBLOCK) {
                if (get_program_time() - last_update_time > c.NETWORK_TIMEOUT_TIME) {
                    return -1;
                }
                task_yield();
                continue;
            } else {
                total_bytes_received += total;
                return -1;
            }
        } else if (r == 0) {
            total_bytes_received += total;
            return @intCast(total);
        }
        total += @intCast(r);
        last_update_time = get_program_time();
    }

    total_bytes_received += total;
    return @intCast(total);
}

pub export fn send_all(client_fd: c_int, buf: ?*const anyopaque, len: isize) isize {
    if (buf == null) return -1;
    const p: [*]const u8 = @ptrCast(buf.?);
    var sent: isize = 0;
    var last_update_time = get_program_time();
    const flags: c_int = if (is_windows) 0 else c.MSG_NOSIGNAL;

    while (sent < len) {
        const n = c.send(client_fd, p + @as(usize, @intCast(sent)), @as(usize, @intCast(len - sent)), flags);
        if (n > 0) {
            sent += n;
            last_update_time = get_program_time();
            continue;
        }
        if (n == 0) {
            set_errno(c.ECONNRESET);
            return -1;
        }
        if (wouldBlockSend()) {
            if (get_program_time() - last_update_time > c.NETWORK_TIMEOUT_TIME) {
                return -1;
            }
            task_yield();
            continue;
        }
        return -1;
    }
    return sent;
}

// Writers (big-endian)
pub export fn writeByte(client_fd: c_int, byte: u8) isize {
    return send_all(client_fd, &byte, 1);
}

pub export fn writeUint16(client_fd: c_int, num: u16) isize {
    var be: u16 = std.mem.nativeToBig(u16, num);
    return send_all(client_fd, &be, @sizeOf(u16));
}

pub export fn writeUint32(client_fd: c_int, num: u32) isize {
    var be: u32 = std.mem.nativeToBig(u32, num);
    return send_all(client_fd, &be, @sizeOf(u32));
}

pub export fn writeUint64(client_fd: c_int, num: u64) isize {
    var be: u64 = std.mem.nativeToBig(u64, num);
    return send_all(client_fd, &be, @sizeOf(u64));
}

pub export fn writeFloat(client_fd: c_int, num: f32) isize {
    const bits: u32 = @bitCast(num);
    var be: u32 = std.mem.nativeToBig(u32, bits);
    return send_all(client_fd, &be, @sizeOf(u32));
}

pub export fn writeDouble(client_fd: c_int, num: f64) isize {
    const bits: u64 = @bitCast(num);
    var be: u64 = std.mem.nativeToBig(u64, bits);
    return send_all(client_fd, &be, @sizeOf(u64));
}

// Readers
pub export fn readByte(ctx: *c.ServerContext, client_fd: c_int) u8 {
    ctx.recv_count = recv_all(client_fd, &ctx.recv_buffer[0], 1, 0);
    return ctx.recv_buffer[0];
}

pub export fn readUint16(ctx: *c.ServerContext, client_fd: c_int) u16 {
    ctx.recv_count = recv_all(client_fd, &ctx.recv_buffer[0], 2, 0);
    return std.mem.readInt(u16, ctx.recv_buffer[0..2], .big);
}

pub export fn readInt16(ctx: *c.ServerContext, client_fd: c_int) i16 {
    ctx.recv_count = recv_all(client_fd, &ctx.recv_buffer[0], 2, 0);
    return std.mem.readInt(i16, ctx.recv_buffer[0..2], .big);
}

pub export fn readUint32(ctx: *c.ServerContext, client_fd: c_int) u32 {
    ctx.recv_count = recv_all(client_fd, &ctx.recv_buffer[0], 4, 0);
    return std.mem.readInt(u32, ctx.recv_buffer[0..4], .big);
}

pub export fn readUint64(ctx: *c.ServerContext, client_fd: c_int) u64 {
    ctx.recv_count = recv_all(client_fd, &ctx.recv_buffer[0], 8, 0);
    return std.mem.readInt(u64, ctx.recv_buffer[0..8], .big);
}

pub export fn readInt64(ctx: *c.ServerContext, client_fd: c_int) i64 {
    ctx.recv_count = recv_all(client_fd, &ctx.recv_buffer[0], 8, 0);
    return std.mem.readInt(i64, ctx.recv_buffer[0..8], .big);
}

pub export fn readFloat(ctx: *c.ServerContext, client_fd: c_int) f32 {
    const u: u32 = readUint32(ctx, client_fd);
    return @bitCast(u);
}

pub export fn readDouble(ctx: *c.ServerContext, client_fd: c_int) f64 {
    const u: u64 = readUint64(ctx, client_fd);
    return @bitCast(u);
}

pub export fn readString(ctx: *c.ServerContext, client_fd: c_int) void {
    const length = c.readVarInt(ctx, client_fd);
    const len_u: u32 = @bitCast(length);
    ctx.recv_count = recv_all(client_fd, &ctx.recv_buffer[0], len_u, 0);
    ctx.recv_buffer[@intCast(ctx.recv_count)] = 0; // null-terminate
}

// RNG
pub export fn fast_rand(ctx: *c.ServerContext) u32 {
    ctx.rng_seed ^= ctx.rng_seed << 13;
    ctx.rng_seed ^= ctx.rng_seed >> 17;
    ctx.rng_seed ^= ctx.rng_seed << 5;
    return ctx.rng_seed;
}

pub export fn splitmix64(state: u64) u64 {
    var z = state +% 0x9e3779b97f4a7c15;
    z = (z ^ (z >> 30)) *% 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) *% 0x94d049bb133111eb;
    return z ^ (z >> 31);
}
