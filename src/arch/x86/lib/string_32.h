#ifndef STRING32_H
#define STRING32_H

#include <ferrite/types.h>

/* Optimized functions */
size_t strlen(char const*);

int strcmp(char const*, char const*);

int strncmp(char const*, char const*, size_t);

/* Normal functions */

void* memmove(void*, void const*, size_t);

void* memset(void*, int, size_t);

void* memcpy(void*, void const*, size_t);

char* strchr(char const* s, char c);

char* strrchr(char const* s, char c);

char** split(char const* s, char c);

char* substr(char const* str, u32 start, size_t len);

size_t strlcpy(char* dest, char const* src, size_t n);

char* strnstr(char const* str, char const* to_find, size_t len);

size_t strlcat(char* dst, char const* src, size_t n);

#endif /* STRING32_H */
