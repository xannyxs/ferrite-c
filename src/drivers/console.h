#ifndef CONSOLE_H
#define CONSOLE_H

#include "sys/process.h"

#include <stdint.h>

typedef struct exec {
    char const* cmd;
    void (*f)(void);
} exec_t;

typedef struct {
    uint8_t buf[256];
    int32_t head; // Read position
    int32_t tail; // Write position
    pid_t shell_pid;
} tty_t;

void console_add_buffer(char c);

void console_init(void);

#endif /* CONSOLE_H */
