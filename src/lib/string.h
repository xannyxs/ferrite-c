#ifndef STRING_H
#define STRING_H

#include "types.h"

size_t strlen(char const* s);

void* memcpy(void* dest, void const* src, size_t n);

void* memmove(void* dest, void const* src, size_t len);

void memset(void* s, int c, size_t n);

s32 strcmp(char const* s1, char const* s2);

s32 strncmp(char const* str1, char const* str2, size_t n);

char** split(char const* s, char c);

char* substr(char const* str, u32 start, size_t len);

size_t strlcpy(char* dest, char const* src, size_t n);

#endif /* STRING_H */
