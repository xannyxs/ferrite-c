const c = @cImport({
    @cInclude("lib/stdlib.h");
});

const PORT: u16 = 0x3f8;

const inb = @import("io.zig").inb;
const outb = @import("io.zig").outb;

pub export fn serial_write_byte(a: u8) void {
    while ((inb(PORT + 5) & 0x20) == 0) {}
    outb(PORT, a);
}

pub export fn serial_init() void {
    outb(PORT + 1, 0x00); // Disable all interrupts
    outb(PORT + 3, 0x80); // Enable DLAB (set baud rate divisor)
    outb(PORT, 0x03); // Set divisor to 3 (lo byte) 38400 baud
    outb(PORT + 1, 0x00); //                  (hi byte)
    outb(PORT + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(PORT + 2, 0xc7); // Enable FIFO, clear them, with 14-byte threshold
    outb(PORT + 4, 0x0b); // IRQs enabled, RTS/DSR set
    outb(PORT + 4, 0x1e); // Set in loopback mode, test the serial chip
    outb(PORT, 0xae); // Test serial chip (send byte 0xAE and check if serial returns same byte)

    if (inb(PORT) != 0xae) {
        c.abort(@constCast("Port unusable"));
    }

    outb(PORT + 4, 0x0f);
}
