#include <arch/x86/io.h>
#include <lib/stdlib.h>

u16 const PORT = 0x3f8;

void serial_write_byte(u8 a)
{
    while ((inb(PORT + 5) & 0x20) == 0) { }
    outb(PORT, a);
}

void serial_init(void)
{
    outb(PORT + 1, 0x00); // Disable all interrupts
    outb(PORT + 3, 0x80); // Enable DLAB (set baud rate divisor)
    outb(PORT, 0x03);     // Set divisor to 3 (lo byte) 38400 baud
    outb(PORT + 1, 0x00); //                  (hi byte)
    outb(PORT + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(PORT + 2, 0xc7); // Enable FIFO, clear them, with 14-byte threshold
    outb(PORT + 4, 0x0b); // IRQs enabled, RTS/DSR set
    outb(PORT + 4, 0x1e); // Set in loopback mode, test the serial chip
    outb(PORT, 0xae); // Test serial chip (send byte 0xAE and check if serial
                      // returns same byte)

    if (inb(PORT) != 0xae) {
        abort("Port unusable");
    }

    outb(PORT + 4, 0x0f);
}
