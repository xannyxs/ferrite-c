#include <stdbool.h>

#if defined(__IS_LIBK)
#include "drivers/printk.h"
#endif

__attribute__((__noreturn__)) void abort(char *err) {
#if defined(__IS_LIBK)
  printk("kernel: panic: %s\n", err);
  printk("abort()\n");

  __asm__ __volatile__("hlt");
#else
  (void)err;
  // TODO: Abnormally terminate the process as if by SIGABRT.
#endif

  while (true) {
  }
  __builtin_unreachable();
}
