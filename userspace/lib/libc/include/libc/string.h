#ifndef STRING_H
#define STRING_H

#include <uapi/types.h>

#define memcpy(dest, src, n) __builtin_memcpy((dest), (src), (n))
#define memset(s, c, n) __builtin_memset((s), (c), (n))

int strlen(char const*);

int strcmp(char const*, char const*);

int strncmp(char const*, char const*, size_t);

size_t strlcat(char*, char const*, size_t);

size_t strlcpy(char*, char const*, size_t);

#endif
