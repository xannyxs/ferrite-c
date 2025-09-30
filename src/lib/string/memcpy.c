#include "types.h"

void* memcpy(void* dest, void const* src, size_t n)
{
    u8* d = dest;
    u8 const* s = src;

    if (src || dest) {
        while (n) {
            n--;
            *d = *s;
            d++;
            s++;
        }
    }
    return (dest);
}
