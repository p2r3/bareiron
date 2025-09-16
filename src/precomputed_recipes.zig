const std = @import("std");
const recipes_mod = @import("recipes.zig");

comptime {
    @setEvalBranchQuota(200_000);
}

pub const Output = struct {
    item: u16,
    count: u8,
    idx: u16,
};

pub const ShapedEntry = struct {
    key: [9]u16,
    out: Output,
};

pub const ShapelessEntry = struct {
    item: u16,
    count: u8,
    out: Output,
};

fn countShapedEntries() usize {
    var total: usize = 0;
    inline for (recipes_mod.recipes) |r| {
        if (r.shapeless_count == 0) {
            const w: usize = r.width;
            const h: usize = r.height;
            total += (3 - w + 1) * (3 - h + 1);
        }
    }
    return total;
}

fn countShapelessEntries() usize {
    var total: usize = 0;
    inline for (recipes_mod.recipes) |r| {
        if (r.shapeless_count > 0) total += 1;
    }
    return total;
}

fn ShapedEntriesType() type {
    return [countShapedEntries()]ShapedEntry;
}

fn ShapelessEntriesType() type {
    return [countShapelessEntries()]ShapelessEntry;
}

fn buildShapedEntries() ShapedEntriesType() {
    comptime {
        @setEvalBranchQuota(500_000);
    }
    var arr: ShapedEntriesType() = undefined;
    var i: usize = 0;
    inline for (recipes_mod.recipes, 0..) |r, idx| {
        if (r.shapeless_count != 0) continue;

        const w: usize = r.width;
        const h: usize = r.height;
        const max_row = 3 - h;
        const max_col = 3 - w;

        var start_row: usize = 0;
        while (start_row <= max_row) : (start_row += 1) {
            var start_col: usize = 0;
            while (start_col <= max_col) : (start_col += 1) {
                var grid: [9]u16 = .{0} ** 9;
                var rr: usize = 0;
                while (rr < h) : (rr += 1) {
                    var cc: usize = 0;
                    while (cc < w) : (cc += 1) {
                        const recipe_idx = rr * w + cc;
                        const gidx = (start_row + rr) * 3 + (start_col + cc);
                        grid[gidx] = r.shape[recipe_idx];
                    }
                }

                arr[i] = .{ .key = grid, .out = .{ .item = r.output_item, .count = r.output_count, .idx = @intCast(idx) } };
                i += 1;
            }
        }
    }

    const less = struct {
        fn lt(_: void, a: ShapedEntry, b: ShapedEntry) bool {
            var k: usize = 0;
            while (k < 9) : (k += 1) {
                if (a.key[k] < b.key[k]) return true;
                if (a.key[k] > b.key[k]) return false;
            }
            return false;
        }
    };
    std.mem.sort(ShapedEntry, &arr, {}, less.lt);
    return arr;
}

fn buildShapelessEntries() ShapelessEntriesType() {
    comptime {
        @setEvalBranchQuota(200_000);
    }
    var arr: ShapelessEntriesType() = undefined;
    var i: usize = 0;
    inline for (recipes_mod.recipes, 0..) |r, idx| {
        if (r.shapeless_count == 0) continue;
        arr[i] = .{ .item = r.shape[0], .count = r.shapeless_count, .out = .{ .item = r.output_item, .count = r.output_count, .idx = @intCast(idx) } };
        i += 1;
    }

    const less = struct {
        fn lt(_: void, a: ShapelessEntry, b: ShapelessEntry) bool {
            if (a.item == b.item) return a.count < b.count;
            return a.item < b.item;
        }
    };
    std.mem.sort(ShapelessEntry, &arr, {}, less.lt);
    return arr;
}

pub const shaped_entries = buildShapedEntries();
pub const shapeless_entries = buildShapelessEntries();

fn compareKeys(a: [9]u16, b: [9]u16) std.math.Order {
    var i: usize = 0;
    while (i < 9) : (i += 1) {
        if (a[i] < b[i]) return .lt;
        if (a[i] > b[i]) return .gt;
    }
    return .eq;
}

pub fn shapedLookup(grid: [9]u16) ?Output {
    var lo: isize = 0;
    var hi: isize = @as(isize, @intCast(shaped_entries.len)) - 1;
    while (lo <= hi) {
        const mid = lo + ((hi - lo) >> 1);
        const e = shaped_entries[@intCast(mid)];
        switch (compareKeys(grid, e.key)) {
            .lt => hi = mid - 1,
            .gt => lo = mid + 1,
            .eq => return e.out,
        }
    }
    return null;
}

pub fn shapelessLookup(item: u16, count: u8) ?Output {
    var lo: isize = 0;
    var hi: isize = @as(isize, @intCast(shapeless_entries.len)) - 1;
    while (lo <= hi) {
        const mid = lo + ((hi - lo) >> 1);
        const e = shapeless_entries[@intCast(mid)];
        if (item == e.item) {
            if (count == e.count) return e.out;
            if (count < e.count) hi = mid - 1 else lo = mid + 1;
        } else if (item < e.item) {
            hi = mid - 1;
        } else {
            lo = mid + 1;
        }
    }
    return null;
}
