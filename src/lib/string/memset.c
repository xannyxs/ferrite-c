#include "types.h"

void* memset(void* str, int c, size_t len)
{
    size_t i = 0;
    unsigned char* s = str;

    for (; len; len--) {
        s[i] = c;
        i++;
    }

    return s;
}
