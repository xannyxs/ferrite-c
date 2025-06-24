#ifndef STRING_H
#define STRING_H

#include <stddef.h>

static inline size_t strlen(const char *s);

void *memmove(void *dest, const void *src, size_t len);

void memset(void *s, int c, size_t n);

void strcmp(const char *s1, const char *s2);

#endif /* STRING_H */
