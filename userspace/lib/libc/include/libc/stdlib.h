#ifndef _LIBC_STDLIB_H
#define _LIBC_STDLIB_H

#include <uapi/types.h>

void* malloc(size_t);
void free(void*);

int atoi(char const* str);

#endif
