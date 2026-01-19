#include "drivers/printk.h"

__attribute__((__noreturn__)) void abort(char* err)
{
    printk("kernel: panic: %s\n", err);
    printk("abort()\n");
    __asm__ __volatile__("hlt");

    while (1) { }
    __builtin_unreachable();
}
