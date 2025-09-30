#ifndef STRING_H
#define STRING_H

#include "types.h"

size_t strlen(char const* s);

void* memcpy(void* dest, void const* src, size_t n);

void* memmove(void* dest, void const* src, size_t len);

void memset(void* s, int c, size_t n);

s32 strcmp(char const* s1, char const* s2);

size_t strlcpy(char* dest, char const* src, size_t n);

#endif /* STRING_H */
