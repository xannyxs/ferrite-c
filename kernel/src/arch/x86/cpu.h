#ifndef CPU_H
#define CPU_H

#include "arch/x86/io.h"

#include <types.h>

static inline void halt(void) { __asm__ __volatile__("hlt"); }

static inline __attribute__((noreturn)) void reboot(void)
{
    u8 good = 0x02;

    while (good == 0x02) {
        good = inb(0x64);
    }

    outb(0x64, 0xfe);

    halt();
    __builtin_unreachable();
}

#endif /* CPU_H */
