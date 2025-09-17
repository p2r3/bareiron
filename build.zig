const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "bareiron-zig",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .target = target,
            .optimize = optimize,
        }),
    });

    exe.linkLibC();

    exe.root_module.addCSourceFiles(.{
        .files = &.{
            "src/dispatch.c",
            "src/dispatch_play_movement.c",
            "src/dispatch_play_chat.c",
            "src/dispatch_play_inventory.c",
            "src/dispatch_play_system.c",
            "src/packets.c",
            "src/procedures.c",
            "src/registries.c",
        },
        .flags = &.{},
    });

    exe.root_module.addIncludePath(b.path("include"));

    switch (target.result.os.tag) {
        .windows => {
            exe.linkSystemLibrary("ws2_32");
        },
        .linux => {
            exe.linkSystemLibrary("c");
            exe.linkSystemLibrary("rt");
        },
        else => {
            exe.linkSystemLibrary("c");
        },
    }

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    const run_step = b.step("run", "Run the application");
    run_step.dependOn(&run_cmd.step);
}
