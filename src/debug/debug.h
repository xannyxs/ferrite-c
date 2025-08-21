#ifndef DEBUG_H
#define DEBUG_H

#if defined(__BOCHS)
#include "arch/x86/io.h"

#define DEBUG_BREAK() __asm__ __volatile__("xchg %bx, %bx")

static inline void bochs_print_string(const char *str) {
  for (int i = 0; str[i] != '\0'; i++) {
    outb(0xe9, str[i]);
  }
}
#else
#define DEBUG_BREAK()                                                          \
  do {                                                                         \
  } while (0)

static inline void bochs_print_string(const char *str) { (void)str; }
#endif /* BOCHS */

#endif /* DEBUG_H */
