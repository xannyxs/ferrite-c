#include "arch/x86/gdt/gdt.h"
#include "arch/x86/pic.h"
#include "arch/x86/time/rtc.h"
#include "debug/debug.h"
#include "drivers/console.h"
#include "drivers/printk.h"
#include "drivers/video/vga.h"
#include "lib/stdlib.h"
#include "memory/buddy_allocator/buddy.h"
#include "memory/kmalloc.h"
#include "memory/memblock.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "sys/tasks.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

__attribute__((noreturn)) void kmain(uint32_t magic, multiboot_info_t *mbd) {
  if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
    abort("Invalid magic number!");
  }

  gdt_init();
  idt_init();
  pic_remap(0x20, 0x28);

  vga_init();
  rtc_init();

  pmm_init_from_map(mbd);
  memblock_init();
  buddy_init();
  vmm_init_pages();
  kmalloc_init();

  char *str = kmalloc(10);
  memcpy(str, "Hello!", 10);
  printk("%s\n", str);

  kfree(str);

  console_init();

  __asm__ volatile("sti");

  while (true) {
    run_scheduled_tasks();

    __asm__ volatile("hlt");
  }

  __builtin_unreachable();
}
