#include <drivers/printk.h>
#include <types.h>

void* memcpy(void* dest, void const* src, size_t n)
{
    int d0;
    int d1;
    int d2;

    if (n == 0 || src == dest) {
        return dest;
    }

    __asm__ volatile("cld\n\t"
                     "movl %%edx, %%ecx\n\t"
                     "shrl $2, %%ecx\n\t"
                     "rep; movsl\n\t"
                     "testb $1, %%dl\n\t"
                     "je 1f\n\t"

                     "movsb\n"
                     "1:\ttestb $2,%%dl\n\t"
                     "je 2f\n\t"
                     "movsw\n"

                     "2:\n"

                     : "=&c"(d0), "=&D"(d1), "=&S"(d2)
                     : "d"(n), "1"(dest), "2"(src)
                     : "memory");

    return dest;
}
