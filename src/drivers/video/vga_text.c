#include "vga.h"

#include <ferrite/types.h>
#include <ferrite/string.h>

size_t terminal_row;
size_t terminal_column;
vga_color_t terminal_color;
u16* terminal_buffer = (u16*)VGA_MEMORY;

/* Private */

static inline u8 vga_entry_color(enum vga_color fg, enum vga_color bg)
{
    return fg | bg << 4;
}

__attribute__((target("general-regs-only"))) static inline u16
vga_entry(u8 uc, u8 color)
{
    return (u16)uc | (u16)color << 8;
}

__attribute__((target("general-regs-only"))) static void vga_scroll_up(void)
{
    memmove(
        terminal_buffer, terminal_buffer + VGA_WIDTH,
        sizeof(u16) * VGA_WIDTH * (VGA_HEIGHT - 1)
    );

    for (size_t x = 0; x < VGA_WIDTH; x++) {
        size_t const index = ((VGA_HEIGHT - 1) * VGA_WIDTH) + x;
        terminal_buffer[index] = vga_entry(' ', terminal_color);
    }
}

/* Public */

void vga_init(void)
{
    terminal_column = 0;
    terminal_row = VGA_HEIGHT - 1;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            size_t const index = (y * VGA_WIDTH) + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

void vga_setcolor(vga_color_t color) { terminal_color = color; }

__attribute__((target("general-regs-only"))) void
vga_putentryat(char c, u8 color, size_t x, size_t y)
{
    size_t const index = (y * VGA_WIDTH) + x;
    terminal_buffer[index] = vga_entry(c, color);
}

void vga_clear_char(void)
{
    if (terminal_column > 0) {
        terminal_column--;
        vga_putentryat(' ', terminal_color, terminal_column, terminal_row);
    }
}

__attribute__((target("general-regs-only"))) void vga_putchar(char c)
{
    if (c == '\n') {
        vga_scroll_up();
        terminal_column = 0;
        return;
    }

    vga_putentryat(c, terminal_color, terminal_column, terminal_row);
    terminal_column++;

    if (terminal_column >= VGA_WIDTH) {
        vga_scroll_up();
        terminal_column = 0;
    }
}

__attribute__((target("general-regs-only"))) void
vga_write(char const* data, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        vga_putchar(data[i]);
    }
}

__attribute__((target("general-regs-only"))) void
vga_writestring(char const* data)
{
    vga_write(data, strlen(data));
}
