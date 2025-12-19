#ifndef STRING32_H
#define STRING32_H

#include <ferrite/types.h>

/* Builtin functions */
extern void* memcpy(void*, void const*, size_t);
extern void memset(void*, int, size_t);

#define memcpy(dest, src, n) __builtin_memcpy(dest, src, n)
#define memset(src, c, n) __builtin_memset(src, c, n)

/* Internal functions */
size_t strlen(char const*);

void* memmove(void*, void const*, size_t);

int strcmp(char const*, char const*);

int strncmp(char const*, char const*, size_t);

// TODO still

char* strchr(char const* s, char c);

char* strrchr(char const* s, char c);

char** split(char const* s, char c);

char* substr(char const* str, u32 start, size_t len);

size_t strlcpy(char* dest, char const* src, size_t n);

char* strnstr(char const* str, char const* to_find, size_t len);

size_t strlcat(char* dst, char const* src, size_t n);

#endif /* STRING32_H */
