#ifndef _LIBC_STDIO_H
#define _LIBC_STDIO_H

#include <uapi/types.h>

__attribute__((format(printf, 1, 2))) __attribute__((noinline)) int
printf(char const*, ...);

__attribute__((format(printf, 3, 4))) __attribute__((noinline)) int
snprintf(char*, size_t, char const*, ...);

#endif
