#include "arch/x86/gdt/gdt.h"
#include "arch/x86/multiboot.h"
#include "drivers/console.h"
#include "drivers/keyboard.h"
#include "drivers/printk.h"
#include "drivers/video/vga.h"
#include "lib/stdlib.h"
#include "memory/kmalloc.h"
#include "memory/pmm.h"
#include "memory/vmm.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__linux__)
#error "You are not using a cross-compiler"
#endif

#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

__attribute__((noreturn)) void kmain(uint32_t magic, multiboot_info_t *mbd) {
  gdt_init();
  vga_init();

  if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
    abort("invalid magic number!");
  }

  pmm_init_from_map(mbd);
  vmm_init_pages();

  kmalloc_init();

  char *str = kmalloc(10);
  str[0] = 'H';
  str[1] = 'A';
  str[2] = 'L';
  str[3] = 'L';
  str[4] = 'O';
  str[5] = '\0';

  kfree(str);

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
