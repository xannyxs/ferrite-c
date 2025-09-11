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

extern bool KMALLOC_INIT;

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

extern void trapret(void);

void create_first_process(void) {
  proc_t *p = proc_alloc();
  if (!p) {
    abort("Fatal: Could not allocate first process.");
  }

  p->kstack = kalloc(KSTACKSIZE);
  if (!p->kstack) {
    abort("Fatal: Could not allocate kstack for first process.");
  }

  char *sp = p->kstack + KSTACKSIZE;

  // Leave room for trapframe
  sp -= sizeof(trapframe_t);
  p->trap = (trapframe_t *)sp;

  // Push a fake return address (we'll never actually return here)
  sp -= 4;
  *(uint32_t *)sp = 0; // Null return address

  // Now push a context that will "return" to kernel_thread_test
  sp -= 4;
  *(uint32_t *)sp =
      (uint32_t)kernel_thread_test; // This becomes the return address for swtch

  // Push dummy values for the saved registers
  sp -= 16; // Space for ebp, ebx, esi, edi
  memset(sp, 0, 16);

  p->context = (context_t *)sp;
  p->state = RUNNABLE;
}

extern void swtch(context_t **, context_t *);
extern cpu_t cpus[NCPU];

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
  KMALLOC_INIT = true;
  vmalloc_init();

  cpu_init();
  proc_init();

  create_first_process();

  console_init();
  __asm__ volatile("sti");

  context_t *dummy_context = NULL;
  swtch(&dummy_context, cpus[0].scheduler);

  printk("kmain: ERROR! Returned from scheduler swtch!\n");
  __builtin_unreachable();
}
