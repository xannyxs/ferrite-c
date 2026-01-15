const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.resolveTargetQuery(.{
        .cpu_arch = .x86,
        .os_tag = .freestanding,
    });

    const optimize = .ReleaseSmall;

    // Build options
    const options = b.addOptions();
    options.addOption(bool, "print_serial", b.option(bool, "print_serial", "Enable serial output") orelse false);
    options.addOption(bool, "debug", b.option(bool, "debug", "Enable debug") orelse true);

    // Find all .zig files in drivers/src/
    var dir = std.fs.cwd().openDir("drivers/src", .{ .iterate = true }) catch @panic("Failed to open drivers/src");
    defer dir.close();

    var iter = dir.iterate();
    while (iter.next() catch @panic("Failed to iterate")) |entry| {
        if (entry.kind == .file and std.mem.endsWith(u8, entry.name, ".zig")) {
            const name = entry.name[0 .. entry.name.len - 4]; // Remove .zig extension
            const source_path = b.fmt("drivers/src/{s}", .{entry.name});

            b.addObject(.{ .root_module =  })

            obj.addIncludePath(b.path("include"));
            obj.addIncludePath(b.path("src"));

            b.installArtifact(obj);
        }
    }
}
