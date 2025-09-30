#ifndef DEBUG_H
#define DEBUG_H

#if defined(__bochs)

#    include "arch/x86/io.h"

#    define BOCHS_MAGICBREAK() __asm__ __volatile__("xchg %bx, %bx");

static inline void bochs_print_string(char const* str)
{
    for (int i = 0; str[i] != '\0'; i++) {
        outb(0xe9, str[i]);
    }
}

#endif /* BOCHS */

#endif /* DEBUG_H */
