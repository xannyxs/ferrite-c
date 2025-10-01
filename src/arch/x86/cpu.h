#pragma once

#include "arch/x86/io.h"
#include "types.h"

inline void halt(void) { __asm__ __volatile__("hlt"); }

inline void halt_loop(void)
{
    while (1) {
        halt();
    }
}

__attribute__((noreturn)) void reboot(void)
{
    u8 good = 0x02;

    while (good == 0x02) {
        good = inb(0x64);
    }

    outb(0x64, 0xfe);

    halt_loop();
    __builtin_unreachable();
}
