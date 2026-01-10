const c = @cImport({
    @cInclude("drivers/vga.h");
});

const VGA_WIDTH: usize = c.VGA_WIDTH;
const VGA_HEIGHT: usize = c.VGA_HEIGHT;

const VGA_MEMORY = 0xC00B8000;

const VgaColour = enum(u8) {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

fn vgaEntryColour(fg: VgaColour, bg: VgaColour) u8 {
    return @intFromEnum(fg) | (@intFromEnum(bg) << 4);
}

fn vgaEntry(ch: u8, colour: u8) u16 {
    return @as(u16, ch) | (@as(u16, colour) << 8);
}

pub const Writer = struct {
    column: usize,
    row: usize,
    colour: u8,
    buffer: [*]volatile u16,

    pub fn new() Writer {
        return Writer{
            .column = 0,
            .row = 0,
            .colour = vgaEntryColour(.VGA_COLOR_WHITE, .VGA_COLOR_BLACK),
            .buffer = @ptrFromInt(VGA_MEMORY),
        };
    }

    pub fn init(self: *Writer) void {
        self.clearScreen();
    }

    pub fn writeString(self: *Writer, str: [*:0]const u8) void {
        var i: usize = 0;

        while (str[i] != 0) : (i += 1) {
            self.writeChar(str[i]);
        }
    }

    pub fn writeChar(self: *Writer, ch: u8) void {
        if (ch == '\n') {
            self.newLine();
            return;
        }

        const index = self.row * VGA_WIDTH + self.column;
        self.buffer[index] = vgaEntry(ch, self.colour);

        self.column += 1;
        if (self.column >= VGA_WIDTH) {
            self.newLine();
        }
    }

    pub fn clearScreen(self: *Writer) void {
        for (0..VGA_HEIGHT) |row| {
            for (0..VGA_WIDTH) |col| {
                const index = (row * VGA_WIDTH) + col;
                self.buffer[index] = vgaEntry(' ', self.colour);
            }
        }

        self.column = 0;
        self.row = 0;
    }

    pub fn clearChar(self: *Writer) void {
        if (self.column > 0) {
            self.column -= 1;
            const index = self.row * VGA_WIDTH + self.column;
            self.buffer[index] = vgaEntry(' ', self.colour);
        }
    }

    fn newLine(self: *Writer) void {
        self.column = 0;

        if (self.row < VGA_HEIGHT - 1) {
            self.row += 1;
            return;
        }

        self.scroll();
    }

    fn scroll(self: *Writer) void {
        for (1..VGA_HEIGHT) |row| {
            for (0..VGA_WIDTH) |col| {
                const src_index = row * VGA_WIDTH + col;
                const dst_index = (row - 1) * VGA_WIDTH + col;
                self.buffer[dst_index] = self.buffer[src_index];
            }
        }

        const blank: u16 = vgaEntry(' ', self.colour);
        for (0..VGA_WIDTH) |x| {
            const index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
            self.buffer[index] = blank;
        }
    }
};

var WRITER = Writer.new();

pub export fn vga_init() void {
    WRITER.init();
}

pub export fn vga_putchar(ch: u8) void {
    WRITER.writeChar(ch);
}

pub export fn vga_puts(str: [*:0]const u8) void {
    WRITER.writeString(str);
}

pub export fn vga_clear() void {
    WRITER.clearScreen();
}

pub export fn vga_delete() void {
    WRITER.clearChar();
}
