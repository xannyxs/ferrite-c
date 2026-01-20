#include <types.h>

size_t strlen(char const* s)
{
    int d0;
    int __res;

    __asm__ volatile("repne scasb"

                     : "=c"(__res), "=&D"(d0)
                     : "1"(s), "a"(0), "0"(0xffffffffu)
                     : "memory");

    return ~__res - 1;
}
