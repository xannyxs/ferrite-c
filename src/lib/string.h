#ifndef STRING_H
#define STRING_H

#include <stddef.h>

static inline size_t strlen(const char *s);

void *memmove(void *dest, const void *src, size_t len);

#endif /* STRING_H */
