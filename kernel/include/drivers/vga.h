#ifndef VGA_H
#define VGA_H

#include <ferrite/types.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xC00B8000

typedef enum {
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
} VgaColour;

typedef struct {
    size_t column;
    size_t row;
    u8 colour;
    u16 volatile* buffer;
} Writer;

/* VGA API */

void vga_init(void);

void vga_putchar(unsigned char);

void vga_puts(char const*);

void vga_clear(void);

void vga_delete(void);

void vga_setcolour(u8, u8);

#endif /* VGA_H */
