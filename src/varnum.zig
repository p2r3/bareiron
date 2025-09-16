const std = @import("std");
const c = @import("c_api.zig").c;

const SEGMENT_BITS: u8 = 0x7F;
const CONTINUE_BIT: u8 = 0x80;

fn varnum_error() i32 {
    return @bitCast(@as(u32, 0xFFFF_FFFF));
}

pub export fn readVarInt(ctx: *c.ServerContext, client_fd: c_int) i32 {
    var value: u32 = 0;
    var position: u32 = 0;

    while (true) {
        const byte: u8 = c.readByte(ctx, client_fd);
        if (ctx.recv_count != 1) return varnum_error();

        value |= (@as(u32, byte & SEGMENT_BITS)) << @intCast(position);

        if ((byte & CONTINUE_BIT) == 0) break;

        position += 7;
        if (position >= 32) return varnum_error();
    }

    return @bitCast(value);
}

pub export fn sizeVarInt(value: u32) c_int {
    var v = value;
    var size: c_int = 1;
    while ((v & ~@as(u32, SEGMENT_BITS)) != 0) {
        v >>= 7;
        size += 1;
    }
    return size;
}

pub export fn writeVarInt(client_fd: c_int, value_in: u32) void {
    var v = value_in;
    while (true) {
        if ((v & ~@as(u32, SEGMENT_BITS)) == 0) {
            _ = c.writeByte(client_fd, @truncate(v));
            return;
        }

        _ = c.writeByte(client_fd, @as(u8, @truncate(v & SEGMENT_BITS)) | CONTINUE_BIT);
        v >>= 7;
    }
}
