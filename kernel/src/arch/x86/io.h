#ifndef IO_H
#define IO_H

#include <types.h>

static inline u8 inb(u16 addr)
{
    u8 out;
    __asm__ __volatile__("inb %1, %0" : "=a"(out) : "d"(addr));
    return out;
}

static inline void outb(u16 addr, u8 val)
{
    __asm__ __volatile__("outb %1, %0" : : "d"(addr), "a"(val));
}

static inline u16 inw(u16 addr)
{
    u16 out;
    __asm__ __volatile__("inw %1, %0" : "=a"(out) : "d"(addr));
    return out;
}

static inline void outw(u16 addr, u16 val)
{
    __asm__ __volatile__("outw %1, %0" : : "d"(addr), "a"(val));
}

static inline u32 inl(u16 addr)
{
    u32 out;
    __asm__ __volatile__("inl %1, %0" : "=a"(out) : "d"(addr));
    return out;
}

static inline void outl(u16 addr, u32 val)
{
    __asm__ __volatile__("outl %1, %0" : : "d"(addr), "a"(val));
}

static inline void io_wait(void) { outb(0x80, 0); }

static inline void cli(void) { __asm__ volatile("cli"); }

static inline void sti(void) { __asm__ volatile("sti"); }

static inline void lcr3(u32 val)
{
    __asm__ volatile("movl %0, %%cr3" : : "r"(val));
}

static inline void qemu_exit(int status) { outl(0xf4, status); }

#endif /* IO_H */
