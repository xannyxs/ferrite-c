#ifndef DEBUG_H
#define DEBUG_H

#include "arch/x86/io.h"

#define BOCHS_BREAK()                                                          \
  outw(0x8A00, 0x8A00);                                                        \
  outw(0x8A00, 0x08AE0);
#define BOCHS_MAGICBREAK() __asm__ __volatile__("xchg %bx, %bx");

static inline void bochs_print_string(const char *str) {
  for (int i = 0; str[i] != '\0'; i++) {
    outb(0xe9, str[i]);
  }
}

#endif /* DEBUG_H */
