#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <stddef.h>
#include <stdint.h>

__attribute__((__noreturn__)) void abort(char*);

char* lltoa_buf(int64_t n, char* buffer, size_t buffer_size);

#endif
