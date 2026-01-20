#include <types.h>

void* memmove(void* dest, void const* src, size_t len)
{
    int d0, d1, d2;

    if (dest == src || len == 0) {
        return dest;
    }

    if (dest < src) {
        __asm__ volatile("cld\n\t"
                         "rep; movsb"

                         : "=&S"(d0), "=&D"(d1), "=&c"(d2)
                         : "0"(src), "1"(dest), "2"(len)
                         : "memory");
    } else {
        __asm__ volatile("std\n\t"
                         "rep; movsb\n\t"
                         "cld"

                         : "=&S"(d0), "=&D"(d1), "=&c"(d2)
                         : "0"((char const*)src + len - 1),
                           "1"((char*)dest + len - 1), "2"(len)
                         : "memory");
    }

    return dest;
}
