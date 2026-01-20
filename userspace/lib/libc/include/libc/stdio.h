#ifndef STDIO_H
#define STDIO_H

#include "../syscalls.h"

__attribute__((format(printf, 1, 2))) int printf(char const*, ...);

__attribute__((format(printf, 3, 4))) int
snprintf(char*, size_t, char const*, ...);

#endif
