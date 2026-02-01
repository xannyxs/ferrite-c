#ifndef _LIBC_STDLIB_H
#define _LIBC_STDLIB_H

#include <uapi/types.h>

void* malloc(size_t);
void free(void*);
void* calloc(size_t, size_t);
void* realloc(void*, size_t);

int atoi(char const* str);

#endif
