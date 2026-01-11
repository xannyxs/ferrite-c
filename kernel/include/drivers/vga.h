#ifndef VGA_H
#define VGA_H

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

/* zig functions */

extern void vga_init(void);

extern void vga_putchar(unsigned char);

extern void vga_puts(char const*);

extern void vga_clear(void);

extern void vga_delete(void);

#endif /* VGA_H */
