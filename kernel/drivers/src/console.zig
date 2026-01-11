const c = @cImport({
    @cInclude("ferrite/types.h");
    @cInclude("drivers/printk.h");
    @cInclude("sys/signal/signal.h");
    @cInclude("drivers/vga.h");
    @cInclude("drivers/chrdev.h");
});

const ChrdevOps = c.chrdev_ops_t;

extern fn vga_puts(s: [*:0]const u8) void;
extern fn vga_putchar(c: u8) void;
extern fn vga_delete() void;

extern fn register_chrdev(major: c_uint, ops: *const ChrdevOps) c_int;
extern fn get_chrdev(major: c_uint) ?*const ChrdevOps;
extern fn unregister_chrdev(major: c_uint) c_int;

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

var SCAN_BUFFER = ScanBuffer.init();

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

var TTY = Tty.new();

pub export fn console_add_buffer(ch: u8) void {
    TTY.addBuffer(ch);
}

fn console_dev_read(dev: c.dev_t, buf_ptr: ?*anyopaque, count: usize, offset: [*c]c_long) callconv(.c) c_int {
    _ = offset;
    _ = dev;

    const buf: [*]u8 = @ptrCast(@alignCast(buf_ptr.?));
    var read_count: usize = 0;

    while (read_count < count) {
        while (tty_is_empty()) {
            asm volatile ("hlt");
        }

        const ch = tty_read();
        if (ch == 0) break;

        buf[read_count] = ch;
        read_count += 1;

        if (ch == '\n') break;
    }

    return @intCast(read_count);
}

fn console_dev_write(dev: c.dev_t, buf_ptr: ?*const anyopaque, count: usize, offset: [*c]c_long) callconv(.c) c_int {
    _ = offset;
    _ = dev;

    const buf: [*]const u8 = @ptrCast(@alignCast(buf_ptr.?));

    for (0..count) |i| {
        vga_putchar(buf[i]);
    }

    return @intCast(count);
}

const console_ops = ChrdevOps{
    .read = console_dev_read,
    .write = console_dev_write,
    .open = null,
    .close = null,
};

pub export fn console_chrdev_init() void {
    const CONSOLE_MAJOR: c_uint = 5;
    _ = register_chrdev(CONSOLE_MAJOR, &console_ops);
}
