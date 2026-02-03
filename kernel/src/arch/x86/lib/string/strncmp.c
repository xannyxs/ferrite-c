#include <types.h>

int strncmp(char const* _l, char const* _r, size_t n)
{
    int d0;
    int d1;
    int d2;
    int __res;

    __asm__ volatile("1:\t decl %3\n\t"
                     "js 2f\n\t"

                     "lodsb\n\t"
                     "scasb\n\t"
                     "jne 3f\n\t"

                     "testb %%al, %%al\n\t"
                     "jne 1b\n\t"

                     "2:\txorl %%eax,%%eax\n\t"
                     "jmp 4f\n"

                     "3:\tsbbl %%eax,%%eax\n\t"
                     "orb $1,%%al\n"

                     "4:"

                     : "=a"(__res), "=&S"(d0), "=&D"(d1), "=&c"(d2)
                     : "1"(_l), "2"(_r), "3"(n)
                     : "memory");

    return __res;
}
