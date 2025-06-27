#include "arch/x86/gdt/gdt.h"
#include "arch/x86/multiboot.h"
#include "drivers/console.h"
#include "drivers/keyboard.h"
#include "drivers/printk.h"
#include "drivers/video/vga.h"
#include "lib/stdlib.h"

#include "memory/block.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__linux__)
#error "You are not using a cross-compiler"
#endif

#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

__attribute__((noreturn)) void _main(uint32_t magic, multiboot_info_t *mbd) {
  gdt_init();
  vga_init();

  if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
    abort("invalid magic number!");
  }

  pmm_init_from_map(mbd);
  init_memory(mbd->mem_upper * 1024, 0x0);

  console_init();

  while (true) {
    char c = keyboard_input();

    if (c == 0) {
      continue;
    }

    console_add_buffer(c);
  }

  __builtin_unreachable();
}
