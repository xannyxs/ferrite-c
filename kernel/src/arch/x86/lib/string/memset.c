#include <types.h>

void* memset(void* str, int c, size_t len)
{
    int d0, d1, d2;

    __asm__ volatile("cld\n\t"
                     "rep\n\t"
                     "stosb"

                     : "=&c"(d0), "=&a"(d1), "=&D"(d2)
                     : "0"(len), "1"(c), "2"(str)
                     : "memory");

    return str;
}
