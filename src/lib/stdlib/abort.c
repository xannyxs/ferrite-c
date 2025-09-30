#if defined(__is_libk)
#    include "drivers/printk.h"
#endif

__attribute__((__noreturn__)) void abort(char* err)
{
#if defined(__is_libk)
    // TODO: Add proper kernel panic.
    printk("kernel: panic: %s\n", err);
    printk("abort()\n");
    __asm__ __volatile__("hlt");
#else
    // TODO: Abnormally terminate the process as if by SIGABRT.
#endif
    while (1) {
    }
    __builtin_unreachable();
}
