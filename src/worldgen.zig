const std = @import("std");
const c = @import("c_api.zig").c;

pub export var chunk_section: [4096]u8 = [_]u8{0} ** 4096;

fn interpolate(h00: u8, h10: u8, h01: u8, h11: u8, x: c_int, z: c_int) u8 {
    const cs: c_int = c.CHUNK_SIZE;
    const top: u16 = @as(u16, h00) * @as(u16, @intCast(cs - x)) + @as(u16, h10) * @as(u16, @intCast(x));
    const bottom: u16 = @as(u16, h01) * @as(u16, @intCast(cs - x)) + @as(u16, h11) * @as(u16, @intCast(x));
    return @intCast((top * @as(u16, @intCast(cs - z)) + bottom * @as(u16, @intCast(z))) / @as(u16, @intCast(cs * cs)));
}

fn getCornerHeight(hash: u32, biome: u8) u8 {
    var height: u8 = c.TERRAIN_BASE_HEIGHT;
    switch (biome) {
        c.W_mangrove_swamp => {
            height += @truncate(hash % 3);
            height += @truncate((hash >> 4) % 3);
            height += @truncate((hash >> 8) % 3);
            height += @truncate((hash >> 12) % 3);
            if (height < 64) height -= @truncate((hash >> 24) & 3);
        },
        c.W_plains => {
            height += @truncate(hash & 3);
            height += @truncate((hash >> 4) & 3);
            height += @truncate((hash >> 8) & 3);
            height += @truncate((hash >> 12) & 3);
        },
        c.W_desert => {
            height += 4;
            height += @truncate((hash & 3));
            height += @truncate((hash >> 4) & 3);
        },
        c.W_beach => {
            height = 62 - @as(u8, @truncate(hash & 3));
            height -= @as(u8, @truncate((hash >> 4) & 3));
            height -= @as(u8, @truncate((hash >> 8) & 3));
        },
        c.W_snowy_plains => {
            height += @truncate(hash & 7);
            height += @truncate((hash >> 4) & 7);
        },
        else => {},
    }
    return height;
}

fn getHeightAtFromAnchors(rx: c_int, rz: c_int, anchor_ptr: [*]const c.ChunkAnchor) u8 {
    if (rx == 0 and rz == 0) {
        const height = getCornerHeight(anchor_ptr[0].hash, anchor_ptr[0].biome);
        if (height > 67) return height -% 1;
    }
    const step: usize = @intCast(16 / c.CHUNK_SIZE);
    return interpolate(
        getCornerHeight(anchor_ptr[0].hash, anchor_ptr[0].biome),
        getCornerHeight(anchor_ptr[1].hash, anchor_ptr[1].biome),
        getCornerHeight(anchor_ptr[step + 1].hash, anchor_ptr[step + 1].biome),
        getCornerHeight(anchor_ptr[step + 2].hash, anchor_ptr[step + 2].biome),
        rx,
        rz,
    );
}

fn getFeatureFromAnchor(ctx: *c.ServerContext, anchor: c.ChunkAnchor) c.ChunkFeature {
    var feature: c.ChunkFeature = undefined;
    const feature_position: u8 = @truncate(anchor.hash % (c.CHUNK_SIZE * c.CHUNK_SIZE));
    const cs_u8: u8 = @intCast(c.CHUNK_SIZE);
    feature.x = @intCast(@mod(feature_position, cs_u8));
    feature.z = @intCast(@divTrunc(feature_position, cs_u8));

    var skip_feature = false;
    if (anchor.biome != c.W_mangrove_swamp) {
        if (feature.x < 3 or feature.x > c.CHUNK_SIZE - 3) skip_feature = true;
        if (feature.z < 3 or feature.z > c.CHUNK_SIZE - 3) skip_feature = true;
    }

    if (skip_feature) {
        feature.y = 0xFF;
    } else {
        const addx: c_int = @as(c_int, anchor.x) * c.CHUNK_SIZE;
        const addz: c_int = @as(c_int, anchor.z) * c.CHUNK_SIZE;
        feature.x = std.math.lossyCast(c_short, @as(c_int, @intCast(feature.x)) + addx);
        feature.z = std.math.lossyCast(c_short, @as(c_int, @intCast(feature.z)) + addz);
        feature.y = getHeightAtFromHash(
            ctx,
            std.math.mod(c_int, feature.x, c.CHUNK_SIZE) catch unreachable,
            std.math.mod(c_int, feature.z, c.CHUNK_SIZE) catch unreachable,
            anchor.x,
            anchor.z,
            anchor.hash,
            anchor.biome,
        ) +% 1;
        feature.variant = @truncate((anchor.hash >> @as(u5, @intCast((feature.x +% feature.z) & 31))) & 1);
    }
    return feature;
}

fn getTerrainAtFromCache(
    x: c_int,
    y: c_int,
    z: c_int,
    rx: c_int,
    rz: c_int,
    anchor: c.ChunkAnchor,
    feature: c.ChunkFeature,
    height: u8,
) u8 {
    if (y >= 64 and y >= height and feature.y != 255) switch (anchor.biome) {
        c.W_plains => {
            if (feature.y < 64) {
                // no tree generation underwater
            } else {
                if (x == feature.x and z == feature.z) {
                    if (y == feature.y - 1) return c.B_dirt;
                    if (y >= feature.y and y < feature.y -% feature.variant +% 6) return c.B_oak_log;
                }
                const dx: u8 = @intCast(if (x >= feature.x) x - feature.x else feature.x - x);
                const dz: u8 = @intCast(if (z >= feature.z) z - feature.z else feature.z - z);
                if (dx < 3 and dz < 3 and y > feature.y -% feature.variant +% 2 and y < feature.y -% feature.variant +% 5) {
                    if (y == feature.y -% feature.variant +% 4 and dx == 2 and dz == 2) {} else return c.B_oak_leaves;
                }
                if (dx < 2 and dz < 2 and y >= feature.y -% feature.variant +% 5 and y <= feature.y -% feature.variant +% 6) {
                    if (y == feature.y -% feature.variant +% 6 and dx == 1 and dz == 1) {} else return c.B_oak_leaves;
                }
                if (y == height) return c.B_grass_block;
                return c.B_air;
            }
        },
        c.W_desert => {
            if (x == feature.x and z == feature.z) {
                if (feature.variant == 0) {
                    if (y == height +% 1) return c.B_dead_bush;
                } else if (y > height) {
                    if ((height & 1) != 0 and y <= height +% 3) return c.B_cactus;
                    if (y <= height +% 2) return c.B_cactus;
                }
            }
        },
        c.W_mangrove_swamp => {
            if (x == feature.x and z == feature.z and y == 64 and height < 63) return c.B_lily_pad;
            if (y == height +% 1) {
                const dx: c_int = if (x >= feature.x) x - feature.x else feature.x - x;
                const dz: c_int = if (z >= feature.z) z - feature.z else feature.z - z;
                if (dx + dz < 4) return c.B_moss_carpet;
            }
        },
        c.W_snowy_plains => {
            if (x == feature.x and z == feature.z and y == height +% 1 and height >= 64) return c.B_short_grass;
        },
        else => {},
    };

    if (height >= 63) {
        if (y == height) {
            return switch (anchor.biome) {
                c.W_mangrove_swamp => c.B_mud,
                c.W_snowy_plains => c.B_snowy_grass_block,
                c.W_desert, c.W_beach => c.B_sand,
                else => c.B_grass_block,
            };
        }
        if (anchor.biome == c.W_snowy_plains and y == height +% 1) return c.B_snow;
    }

    if (y <= height -% 4) {
        const gap: i8 = @intCast(height -% c.TERRAIN_BASE_HEIGHT);
        if (y < c.CAVE_BASE_DEPTH +% gap and y > c.CAVE_BASE_DEPTH -% gap) return c.B_air;

        var ore_y: u8 = (@as(u8, @intCast(rx & 15)) << 4) | @as(u8, @intCast(rz & 15));
        ore_y ^= ore_y << 4;
        ore_y ^= ore_y >> 5;
        ore_y ^= ore_y << 1;
        ore_y &= 63;

        if (y == ore_y) {
            const ore_probability: u8 = @truncate((anchor.hash >> @as(u5, @intCast(ore_y % 24))) & 255);
            if (y < 15) {
                if (ore_probability < 10) return c.B_diamond_ore;
                if (ore_probability < 12) return c.B_gold_ore;
                if (ore_probability < 15) return c.B_redstone_ore;
            }
            if (y < 30) {
                if (ore_probability < 3) return c.B_gold_ore;
                if (ore_probability < 8) return c.B_redstone_ore;
            }
            if (y < 54) {
                if (ore_probability < 30) return c.B_iron_ore;
                if (ore_probability < 40) return c.B_copper_ore;
            }
            if (ore_probability < 60) return c.B_coal_ore;
            if (y < 5) return c.B_lava;
            return c.B_cobblestone;
        }
        return c.B_stone;
    }

    if (y <= height) {
        return switch (anchor.biome) {
            c.W_desert => c.B_sandstone,
            c.W_mangrove_swamp => c.B_mud,
            c.W_beach => if (height > 64) c.B_sandstone else c.B_dirt,
            else => c.B_dirt,
        };
    }

    if (y == 63 and anchor.biome == c.W_snowy_plains) return c.B_ice;
    if (y < 64) return c.B_water;
    return c.B_air;
}

pub export fn getChunkHash(ctx: *c.ServerContext, x: c_short, z: c_short) u32 {
    const x_bits: u16 = @bitCast(x);
    const z_bits: u16 = @bitCast(z);
    const val: u64 = @as(u64, x_bits) | (@as(u64, z_bits) << 16) | (@as(u64, ctx.world_seed) << 32);
    return @as(u32, @truncate(c.splitmix64(val)));
}

pub export fn getChunkBiome(ctx: *c.ServerContext, x_in: c_short, z_in: c_short) u8 {
    const x: c_int = x_in + c.BIOME_RADIUS;
    const z: c_int = z_in + c.BIOME_RADIUS;

    const dx: i8 = @intCast(c.BIOME_RADIUS - (std.math.mod(c_int, x, c.BIOME_SIZE) catch unreachable));
    const dz: i8 = @intCast(c.BIOME_RADIUS - (std.math.mod(c_int, z, c.BIOME_SIZE) catch unreachable));
    if (@as(c_int, dx) * dx + @as(c_int, dz) * dz > c.BIOME_RADIUS * c.BIOME_RADIUS) return c.W_beach;

    const biome_x: c_int = std.math.divFloor(c_int, x, c.BIOME_SIZE) catch unreachable;
    const biome_z: c_int = std.math.divFloor(c_int, z, c.BIOME_SIZE) catch unreachable;
    const index: c_int = (biome_x & 3) + ((biome_z * 4) & 15);
    const shift: u5 = @intCast((index * 2) & 31);
    return @intCast((ctx.world_seed >> shift) & 3);
}

pub export fn getHeightAtFromHash(
    ctx: *c.ServerContext,
    rx: c_int,
    rz: c_int,
    _x: c_int,
    _z: c_int,
    chunk_hash: u32,
    biome: u8,
) u8 {
    if (rx == 0 and rz == 0) {
        const height = getCornerHeight(chunk_hash, biome);
        if (height > 67) return height -% 1;
    }
    return interpolate(
        getCornerHeight(chunk_hash, biome),
        getCornerHeight(getChunkHash(ctx, std.math.lossyCast(c_short, _x + 1), std.math.lossyCast(c_short, _z)), getChunkBiome(ctx, std.math.lossyCast(c_short, _x + 1), std.math.lossyCast(c_short, _z))),
        getCornerHeight(getChunkHash(ctx, std.math.lossyCast(c_short, _x), std.math.lossyCast(c_short, _z + 1)), getChunkBiome(ctx, std.math.lossyCast(c_short, _x), std.math.lossyCast(c_short, _z + 1))),
        getCornerHeight(getChunkHash(ctx, std.math.lossyCast(c_short, _x + 1), std.math.lossyCast(c_short, _z + 1)), getChunkBiome(ctx, std.math.lossyCast(c_short, _x + 1), std.math.lossyCast(c_short, _z + 1))),
        rx,
        rz,
    );
}

pub export fn getHeightAt(ctx: *c.ServerContext, x: c_int, z: c_int) u8 {
    const _x = std.math.divFloor(c_int, x, c.CHUNK_SIZE) catch unreachable;
    const _z = std.math.divFloor(c_int, z, c.CHUNK_SIZE) catch unreachable;
    const rx = std.math.mod(c_int, x, c.CHUNK_SIZE) catch unreachable;
    const rz = std.math.mod(c_int, z, c.CHUNK_SIZE) catch unreachable;
    const chunk_hash = getChunkHash(ctx, std.math.lossyCast(c_short, _x), std.math.lossyCast(c_short, _z));
    const biome = getChunkBiome(ctx, std.math.lossyCast(c_short, _x), std.math.lossyCast(c_short, _z));
    return getHeightAtFromHash(ctx, rx, rz, _x, _z, chunk_hash, biome);
}

pub export fn getTerrainAt(ctx: *c.ServerContext, x: c_int, y: c_int, z: c_int, anchor: c.ChunkAnchor) u8 {
    if (y > 80) return c.B_air;
    const rx = std.math.mod(c_int, x, c.CHUNK_SIZE) catch unreachable;
    const rz = std.math.mod(c_int, z, c.CHUNK_SIZE) catch unreachable;
    const feature = getFeatureFromAnchor(ctx, anchor);
    const height = getHeightAtFromHash(ctx, rx, rz, anchor.x, anchor.z, anchor.hash, anchor.biome);
    return getTerrainAtFromCache(x, y, z, rx, rz, anchor, feature, height);
}

pub export fn getBlockAt(ctx: *c.ServerContext, x: c_int, y: c_int, z: c_int) u8 {
    if (y < 0) return c.B_bedrock;
    const xs: c_short = std.math.lossyCast(c_short, x);
    const ys: u8 = std.math.lossyCast(u8, y);
    const zs: c_short = std.math.lossyCast(c_short, z);
    const block_change = c.getBlockChange(ctx, xs, ys, zs);
    if (block_change != 0xFF) return block_change;
    const anchor_x: c_short = std.math.lossyCast(c_short, std.math.divFloor(c_int, x, c.CHUNK_SIZE) catch unreachable);
    const anchor_z: c_short = std.math.lossyCast(c_short, std.math.divFloor(c_int, z, c.CHUNK_SIZE) catch unreachable);
    const anchor = c.ChunkAnchor{
        .x = anchor_x,
        .z = anchor_z,
        .hash = getChunkHash(ctx, anchor_x, anchor_z),
        .biome = getChunkBiome(ctx, anchor_x, anchor_z),
    };
    return getTerrainAt(ctx, x, y, z, anchor);
}

pub export fn buildChunkSection(ctx: *c.ServerContext, cx: c_int, cy: c_int, cz: c_int) u8 {
    const anchors_w: usize = 16 / c.CHUNK_SIZE + 1;
    var chunk_anchors: [anchors_w * anchors_w]c.ChunkAnchor = undefined;
    var chunk_features: [256 / (c.CHUNK_SIZE * c.CHUNK_SIZE)]c.ChunkFeature = undefined;
    var chunk_section_height: [16][16]u8 = undefined;

    var anchor_index: usize = 0;
    var feature_index: usize = 0;

    var zi: c_int = cz;
    while (zi < cz + 16 + c.CHUNK_SIZE) : (zi += c.CHUNK_SIZE) {
        var xi: c_int = cx;
        while (xi < cx + 16 + c.CHUNK_SIZE) : (xi += c.CHUNK_SIZE) {
            var anchor: *c.ChunkAnchor = &chunk_anchors[anchor_index];
            anchor.x = std.math.lossyCast(c_short, @divTrunc(xi, c.CHUNK_SIZE));
            anchor.z = std.math.lossyCast(c_short, @divTrunc(zi, c.CHUNK_SIZE));
            anchor.hash = getChunkHash(ctx, anchor.x, anchor.z);
            anchor.biome = getChunkBiome(ctx, anchor.x, anchor.z);
            if (zi != cz + 16 and xi != cx + 16) {
                chunk_features[feature_index] = getFeatureFromAnchor(ctx, anchor.*);
                feature_index += 1;
            }
            anchor_index += 1;
        }
    }

    var iy: usize = 0;
    while (iy < 16) : (iy += 1) {
        var ix: usize = 0;
        while (ix < 16) : (ix += 1) {
            const ai: usize = (ix / @as(usize, @intCast(c.CHUNK_SIZE))) + (iy / @as(usize, @intCast(c.CHUNK_SIZE))) * (16 / c.CHUNK_SIZE + 1);
            const ap: [*]const c.ChunkAnchor = chunk_anchors[ai..].ptr;
            chunk_section_height[ix][iy] = getHeightAtFromAnchors(@intCast(ix % c.CHUNK_SIZE), @intCast(iy % c.CHUNK_SIZE), ap);
        }
    }

    const cs_usize: usize = @intCast(c.CHUNK_SIZE);
    const anchors_stride: usize = 16 / cs_usize + 1;
    const features_stride: usize = 16 / cs_usize;
    var j: usize = 0;
    while (j < 4096) : (j += 8) {
        const y = @as(c_int, @intCast(j / 256)) + cy;
        const rz = (j / 16) % 16;
        const rz_mod = @as(c_int, @intCast(rz % @as(usize, @intCast(c.CHUNK_SIZE))));
        feature_index = ((j % 16) / cs_usize) + (rz / cs_usize) * features_stride;
        const anchor_base: usize = ((j % 16) / cs_usize) + (rz / cs_usize) * anchors_stride;

        var offset: i32 = 7;
        while (offset >= 0) : (offset -= 1) {
            const k = j + @as(usize, @intCast(offset));
            const rx = @as(c_int, @intCast(k % 16));
            const ax = chunk_anchors[anchor_base];
            const feat = chunk_features[feature_index];
            chunk_section[j + 7 - @as(usize, @intCast(offset))] = getTerrainAtFromCache(
                rx + cx,
                y,
                @as(c_int, @intCast(rz)) + cz,
                @intCast(@rem(rx, c.CHUNK_SIZE)),
                rz_mod,
                ax,
                feat,
                chunk_section_height[@intCast(rx)][@intCast(rz)],
            );
        }
    }

    var b_idx: c_int = 0;
    while (b_idx < ctx.block_changes_count) : (b_idx += 1) {
        const bc = ctx.block_changes[@intCast(b_idx)];
        if (bc.block == 0xFF) continue;
        if (bc.block == c.B_torch) continue;
        if (@hasDecl(c, "ALLOW_CHESTS")) {
            if (bc.block == c.B_chest) continue;
        }
        if (bc.x >= cx and bc.x < cx + 16 and bc.y >= cy and bc.y < cy + 16 and bc.z >= cz and bc.z < cz + 16) {
            const dx: u32 = @intCast(bc.x - cx);
            const dy: u32 = @intCast(bc.y - cy);
            const dz: u32 = @intCast(bc.z - cz);
            const address: u32 = dx + (dz << 4) + (dy << 8);
            const index = (address & ~@as(u32, 7)) | (7 - (address & 7));
            chunk_section[index] = bc.block;
        }
    }

    return chunk_anchors[0].biome;
}
