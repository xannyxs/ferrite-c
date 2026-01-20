#ifndef CONSOLE_H
#define CONSOLE_H

#include <types.h>
#include <stdbool.h>

extern void console_add_buffer(char c);

extern unsigned char tty_read(void);

extern void tty_write(u8 scancode);

extern bool tty_is_empty(void);

#endif /* CONSOLE_H */
