#include <types.h>

int strcmp(char const* _l, char const* _r)
{
    int d0, d1;
    int __res;

    __asm__ volatile("1:\t lodsb\n\t"

                     "scasb\n\t"
                     "jne 2f\n\t"

                     "testb %%al, %%al\n\t"
                     "jne 1b\n\t"

                     "xorl %%eax,%%eax\n\t"
                     "jmp 3f\n"

                     "2:\tsbbl %%eax,%%eax\n\t"
                     "orb $1,%%al\n"

                     "3:"

                     : "=a"(__res), "=&S"(d0), "=&D"(d1)
                     : "1"(_l), "2"(_r)
                     : "memory");

    return __res;
}
