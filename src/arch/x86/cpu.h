#include "arch/x86/io.h"

#include <stdint.h>

void halt() { __asm__ __volatile__("hlt"); }

void halt_loop() {
  while (1) {
    halt();
  }
}

__attribute__((noreturn)) void reboot() {
  uint8_t good = 0x02;

  while (good == 0x02) {
    good = inb(0x64);
  }

  outb(0x64, 0xfe);

  halt_loop();
  __builtin_unreachable();
}
