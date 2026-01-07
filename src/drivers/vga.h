#ifndef VGA_H
#define VGA_H

#include <ferrite/types.h>

extern unsigned int VGA_WIDTH;
extern unsigned int VGA_HEIGHT;

extern void vga_putchar(u8);
extern void vga_puts(char const*);
extern void vga_clear(void);

#endif
