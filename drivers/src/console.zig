const c = @cImport({
    @cInclude("drivers/printk.h");
    @cInclude("sys/signal/signal.h");
    @cInclude("drivers/vga.h");
});

const shell = @import("shell.zig");

extern fn vga_puts(s: [*:0]const u8) void;
extern fn vga_putchar(c: u8) void;
extern fn vga_delete() void;

const PROMPT = "[42]$ ";
const VGA_WIDTH = c.VGA_WIDTH;

const ScanBuffer = struct {
    buffer: [256]u8,
    head: usize,
    tail: usize,

    pub fn init() ScanBuffer {
        return ScanBuffer{
            .buffer = [_]u8{0} ** 256,
            .head = 0,
            .tail = 0,
        };
    }
};

pub const Tty = struct {
    cmd_buffer: [256]u8,
    cmd_len: usize,
    shell_pid: i32,

    pub fn new() Tty {
        return Tty{
            .cmd_buffer = [_]u8{0} ** 256,
            .cmd_len = 0,
            .shell_pid = 0,
        };
    }

    pub fn addBuffer(self: *Tty, ch: u8) void {
        switch (ch) {
            '\x03' => {
                vga_puts("^C\n");
                _ = c.do_kill(c.myproc().*.pid, c.SIGINT);
                self.clearLine();
            },
            '\n' => {
                vga_putchar('\n');
                if (self.cmd_len > 0) {
                    self.cmd_buffer[self.cmd_len] = 0;

                    var arg_ptr: ?[*:0]const u8 = null;
                    for (0..self.cmd_len) |i| {
                        if (self.cmd_buffer[i] == ' ') {
                            self.cmd_buffer[i] = 0;
                            if (i + 1 < self.cmd_len) {
                                arg_ptr = @ptrCast(&self.cmd_buffer[i + 1]);
                            }
                            break;
                        }
                    }

                    const cmd: [*:0]const u8 = @ptrCast(&self.cmd_buffer[0]);
                    shell.executeCommand(cmd, arg_ptr);
                }
                self.clearLine();
            },
            '\x08' => {
                if (self.cmd_len > 0) {
                    self.cmd_len -= 1;
                    self.cmd_buffer[self.cmd_len] = 0;
                    vga_delete();
                }
            },
            else => {
                if (self.cmd_len < 255) {
                    self.cmd_buffer[self.cmd_len] = ch;
                    self.cmd_len += 1;
                    vga_putchar(ch);
                }
            },
        }
    }

    fn clearLine(self: *Tty) void {
        self.cmd_len = 0;
        self.cmd_buffer = [_]u8{0} ** 256;
        vga_puts(PROMPT);
    }
};

var SCAN_BUFFER = ScanBuffer.init();
var TTY = Tty.new();

pub export fn tty_write(scancode: u8) void {
    if ((SCAN_BUFFER.tail + 1) % 256 != SCAN_BUFFER.head) {
        SCAN_BUFFER.buffer[SCAN_BUFFER.tail] = scancode;
        SCAN_BUFFER.tail = (SCAN_BUFFER.tail + 1) % 256;
    }
}

pub export fn tty_read() u8 {
    if (SCAN_BUFFER.head == SCAN_BUFFER.tail) {
        return 0;
    }
    const byte = SCAN_BUFFER.buffer[SCAN_BUFFER.head];
    SCAN_BUFFER.head = (SCAN_BUFFER.head + 1) % 256;
    return byte;
}

pub export fn tty_is_empty() bool {
    return SCAN_BUFFER.head == SCAN_BUFFER.tail;
}

pub export fn console_add_buffer(ch: u8) void {
    TTY.addBuffer(ch);
}

pub export fn console_init() void {
    vga_puts(PROMPT);
}
