const std = @import("std");

extern fn c_main() c_int;

pub fn main() !void {
    const code: c_int = c_main();
    if (code != 0) std.process.exit(1);
}
