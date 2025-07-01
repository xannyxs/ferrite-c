#include "arch/x86/gdt/gdt.h"
#include "arch/x86/multiboot.h"
#include "drivers/console.h"
#include "drivers/keyboard.h"
#include "drivers/video/vga.h"
#include "lib/stdlib.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__linux__)
#error "You are not using a cross-compiler"
#endif

#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

__attribute__((noreturn)) void kmain(uint32_t magic, multiboot_info_t mbd) {
  gdt_init();
  vga_init();
  (void)mbd;

  if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
    abort("invalid magic number!");
  }

  console_init();

  while (1) {
    char c = keyboard_input();

    if (c == 0) {
      continue;
    }

    console_add_buffer(c);
  }

  __builtin_unreachable();
}
