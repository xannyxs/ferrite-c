const c = @cImport({
    @cInclude("sys/signal/signal.h");
    @cInclude("drivers/vga.h");
});

extern fn vga_puts(s: [*:0]const u8) void;
extern fn vga_putchar(c: u8) void;
extern fn vga_delete() void;

const PROMPT = "[42]$ ";
const BUFFER_SIZE = 256;

const VGA_WIDTH = c.VGA_WIDTH;

const Tty = struct {
    buffer: [256]u8,
    head: usize,
    tail: usize,

    pub fn new() Tty {
        return Tty{
            .buffer = [_]u8{0} ** 256,
            .head = 0,
            .tail = 0,
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
                self.executeBuffer();
                self.clearLine();
            },
            '\x08' => {
                if (self.tail > self.head) {
                    self.tail -= 1;
                    self.buffer[self.tail] = 0;
                    vga_delete();
                }
            },
            else => {
                if (self.tail < BUFFER_SIZE - 1 and (self.tail - self.head) < VGA_WIDTH - 1) {
                    self.buffer[self.tail] = ch;
                    self.tail += 1;
                    vga_putchar(ch);
                }
            },
        }
    }

    fn executeBuffer(self: *Tty) void {
        // TODO: Parse and execute command
        _ = self;
    }

    fn clearLine(self: *Tty) void {
        self.head = 0;
        self.tail = 0;
        self.buffer = [_]u8{0} ** BUFFER_SIZE;
        vga_puts(PROMPT);
    }
};

var TTY = Tty.new();

pub export fn tty_read() u8 {
    if (TTY.head == TTY.tail) {
        return 0;
    }

    const byte = TTY.buffer[TTY.head];
    TTY.head = (TTY.head + 1) % BUFFER_SIZE;

    return byte;
}

pub export fn tty_write(scancode: u8) void {
    if ((TTY.tail + 1) % BUFFER_SIZE != TTY.head) {
        TTY.buffer[TTY.tail] = scancode;
        TTY.tail = (TTY.tail + 1) % BUFFER_SIZE;
    }
}

pub export fn tty_is_empty() bool {
    return TTY.head == TTY.tail;
}

pub export fn console_add_buffer(ch: u8) void {
    TTY.addBuffer(ch);
}

pub export fn console_init() void {
    vga_puts(PROMPT);
}
