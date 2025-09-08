#include "arch/x86/gdt/gdt.h"
#include "arch/x86/idt/idt.h"
#include "arch/x86/pic.h"
#include "arch/x86/pit.h"
#include "arch/x86/time/rtc.h"
#include "drivers/console.h"
#include "drivers/printk.h"
#include "drivers/video/vga.h"
#include "lib/stdlib.h"
#include "memory/buddy_allocator/buddy.h"
#include "memory/kmalloc.h"
#include "memory/memblock.h"
#include "memory/pmm.h"
#include "memory/vmalloc.h"
#include "memory/vmm.h"
#include "sys/cpu.h"
#include "sys/process.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

void kernel_thread_test(void) {
  printk("First kernel process has started!\n");

  int pid = sys_fork();

  if (pid < 0) {
    printk("fork failed!\n");
  } else if (pid == 0) {
    printk("Hello from the child process!\n");
  } else {
    printk("Hello from the parent process! Child PID is %d\n", pid);
  }

  for (;;) {
    yield();
  }
}

#include "sys/defs.h"
#include <string.h>

void create_first_process(void) {
  proc_t *p;

  p = proc_alloc();
  if (!p) {
    abort("Fatal: Could not allocate first process.");
  }

  p->kstack = kmalloc(KSTACKSIZE);
  if (!p->kstack) {
    abort("Fatal: Could not allocate kstack for first process.");
  }

  char *sp = p->kstack + KSTACKSIZE;

  sp -= sizeof(*p->context);
  p->context = (context_t *)sp;
  memset(p->context, 0, sizeof(*p->context));

  p->context->eip = (uint32_t)kernel_thread_test;
  p->state = RUNNABLE;
}

__attribute__((noreturn)) void kmain(uint32_t magic, multiboot_info_t *mbd) {
  if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
    abort("Invalid magic number!");
  }

  gdt_init();
  idt_init();
  pic_remap(0x20, 0x28);
  set_pit_count(LATCH);

  vga_init();
  rtc_init();

  pmm_init_from_map(mbd);
  memblock_init();

  vmm_init_pages();
  buddy_init();
  memblock_deactivate();
  vmalloc_init();

  cpu_init();
  proc_init();

  create_first_process();

  console_init();

  __asm__ volatile("sti");
  scheduler();

  __builtin_unreachable();
}
