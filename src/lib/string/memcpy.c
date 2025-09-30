#include <stddef.h>
#include <stdint.h>

void* memcpy(void* dest, void const* src, size_t n)
{
    uint8_t* d = dest;
    uint8_t const* s = src;

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
