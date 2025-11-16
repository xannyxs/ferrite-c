#ifndef CONSOLE_H
#define CONSOLE_H

#include "sys/process/process.h"
#include <ferrite/types.h>

typedef struct exec {
    char const* cmd;
    void (*f)(void);
} exec_t;

typedef struct {
    u8 buf[256];
    s32 head; // Read position
    s32 tail; // Write position
    pid_t shell_pid;
} tty_t;

void console_add_buffer(char c);

void console_init(void);

#endif /* CONSOLE_H */
