#include <uapi/types.h>

void* memcpy(void* dst, void const* src, size_t n)
{
    char* d = dst;
    char const* s = src;
    while (n--)
        *d++ = *s++;
    return dst;
}
