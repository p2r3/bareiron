// src/structures.zig

const c = @import("c_api.zig").c;

pub export fn setBlockIfReplaceable(ctx: *c.ServerContext, x: c_short, y: u8, z: c_short, block: u8) void {
    const target = c.getBlockAt(ctx, x, y, z);
    // isReplaceableBlock returns a uint8_t in C (0=false, 1=true)
    if (c.isReplaceableBlock(target) == 0 and target != c.B_oak_leaves) {
        return;
    }
    _ = c.makeBlockChange(ctx, x, y, z, block);
}

pub export fn placeTreeStructure(ctx: *c.ServerContext, x: c_short, y: u8, z: c_short) void {
    // Get a random number for tree height and leaf edges.
    const r = c.fast_rand(ctx);
    const height: u8 = 4 + @as(u8, @truncate(r % 3));

    // Set tree base - replace sapling with log and put dirt below.
    _ = c.makeBlockChange(ctx, x, y - 1, z, c.B_dirt);
    _ = c.makeBlockChange(ctx, x, y, z, c.B_oak_log);

    // Create tree stump.
    var i: u8 = 1;
    while (i < height) : (i += 1) {
        setBlockIfReplaceable(ctx, x, y + i, z, c.B_oak_log);
    }

    // Keep track of leaf corners, determines random number bit shift.
    var t: u8 = 2;

    // First (bottom) leaf layer.
    var leaf_x: i32 = -2;
    while (leaf_x <= 2) : (leaf_x += 1) {
        var leaf_z: i32 = -2;
        while (leaf_z <= 2) : (leaf_z += 1) {
            setBlockIfReplaceable(ctx, @intCast(x + leaf_x), y + height - 3, @intCast(z + leaf_z), c.B_oak_leaves);

            // Randomly skip some corners, emulating vanilla tree shape
            var skip_second_layer = false;
            if ((leaf_x == 2 or leaf_x == -2) and (leaf_z == 2 or leaf_z == -2)) {
                t += 1;
                if ((r >> @truncate(t)) & 1 != 0) {
                    skip_second_layer = true;
                }
            }
            if (!skip_second_layer) {
                setBlockIfReplaceable(ctx, @intCast(x + leaf_x), y + height - 2, @intCast(z + leaf_z), c.B_oak_leaves);
            }
        }
    }

    // Second (top) leaf layer.
    leaf_x = -1;
    while (leaf_x <= 1) : (leaf_x += 1) {
        var leaf_z: i32 = -1;
        while (leaf_z <= 1) : (leaf_z += 1) {
            setBlockIfReplaceable(ctx, @intCast(x + leaf_x), y + height - 1, @intCast(z + leaf_z), c.B_oak_leaves);

            var skip_top_layer = false;
            if ((leaf_x == 1 or leaf_x == -1) and (leaf_z == 1 or leaf_z == -1)) {
                t += 1;
                if ((r >> @truncate(t)) & 1 != 0) {
                    skip_top_layer = true;
                }
            }

            if (!skip_top_layer) {
                setBlockIfReplaceable(ctx, @intCast(x + leaf_x), y + height, @intCast(z + leaf_z), c.B_oak_leaves);
            }
        }
    }
}
