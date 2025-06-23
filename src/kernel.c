#include "drivers/console.h"
#include "drivers/video/vga.h"

#include <stdbool.h>
#include <stddef.h>

#if defined(__linux__)
#error "You are not using a cross-compiler"
#endif

#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

void _main(void) {
  vga_init();

  kprintf("Hello\n");
  kprintf("world");
}
