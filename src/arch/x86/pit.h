#ifndef PIT_H
#define PIT_H

#include "arch/x86/io.h"
#include "types.h"

#ifndef HZ
#    define HZ 100
#endif

#define SHIFT_HZ 7
#define SHIFT_SCALE 24 /* shift for phase scale factor */

#define CLOCK_TICK_RATE 1193180 /* Underlying HZ */
#define CLOCK_TICK_FACTOR 20    /* Factor of both 1000000 and CLOCK_TICK_RATE */
#define LATCH ((CLOCK_TICK_RATE + HZ / 2) / HZ)

#define FINETUNE                                    \
    (((((LATCH * HZ - CLOCK_TICK_RATE) << SHIFT_HZ) \
       * (1000000 / CLOCK_TICK_FACTOR)              \
       / (CLOCK_TICK_RATE / CLOCK_TICK_FACTOR))     \
      << (SHIFT_SCALE - SHIFT_HZ))                  \
     / HZ)

static inline void set_pit_count(u32 const count)
{
    __asm__ __volatile__("cli");

    outb(0x40, count & 0xFF);
    outb(0x40, (count & 0xFF00) >> 8);

    __asm__ __volatile__("sti");
}

#endif /* PIT_H */
