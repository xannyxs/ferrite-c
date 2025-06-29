#include "arch/x86/gdt/gdt.h"
#include "arch/x86/multiboot.h"
#include "debug/debug.h"
#include "drivers/console.h"
#include "drivers/keyboard.h"
#include "drivers/printk.h"
#include "drivers/video/vga.h"
#include "lib/stdlib.h"
#include "memory/block.h"
#include "memory/kmalloc.h"
#include "memory/pmm.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__linux__)
#error "You are not using a cross-compiler"
#endif

#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

void test_single_allocation() {
  printk("--- Test 1: Single Allocation ---\n");

  bochs_print_string("Before KMalloc\n");
  BOCHS_MAGICBREAK();

  char *ptr = (char *)kmalloc(14);

  bochs_print_string("After KMalloc\n");
  BOCHS_MAGICBREAK();

  if (ptr == NULL) {
    printk("FAIL: kmalloc returned NULL on first allocation.\n");
    return;
  }

  printk("  kmalloc(16) returned address: 0x%x\n", (uint32_t)ptr);

  // Test if we can write to and read from the memory
  for (int i = 0; i < 16; i++) {
    ptr[i] = (char)('A' + i);
  }

  if (ptr[0] == 'A' && ptr[15] == 'P') {
    printk("PASS: Data integrity check passed.\n");
  } else {
    printk("FAIL: Data written was not read back correctly.\n");
  }

  BOCHS_MAGICBREAK();
}

__attribute__((noreturn)) void _main(uint32_t magic, multiboot_info_t *mbd) {
  gdt_init();
  vga_init();

  if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
    abort("invalid magic number!");
  }

  pmm_init_from_map(mbd);
  init_memory();

  test_single_allocation();
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
