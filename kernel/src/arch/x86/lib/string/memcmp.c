#include <types.h>

int memcmp(void const* _l, void const* _r, size_t len)
{
    int d0;
    int d1;
    int d2;
    int __res;

    __asm__ volatile("cld\n\t"
                     "repe\n\t"
                     "cmpsb\n\t"
                     "je 1f\n\t"
                     "movl $1,%%eax\n\t"
                     "jb 1f\n\t"
                     "negl %%eax\n"
                     "1:"

                     : "=a"(__res), "=&c"(d0), "=&D"(d1), "=&S"(d2)
                     : "0"(0), "2"(_l), "3"(_r), "1"(len)
                     : "memory");
    return __res;
}
