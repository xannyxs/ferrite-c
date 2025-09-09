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
  proc_t *p;

  p = proc_alloc();
  if (!p) {
    abort("Fatal: Could not allocate first process.");
  }

  p->kstack = kalloc(KSTACKSIZE * 2);
  if (!p->kstack) {
    abort("Fatal: Could not allocate kstack for first process.");
  }

  // --- DIAGNOSTIC PRINTS ADDED ---
  printk("--- Creating first process ---\n");
  printk("  -> Kernel thread function is at address: 0x%x\n",
         (uint32_t)kernel_thread_test);
  printk("  -> sizeof(context_t) is: %d bytes\n", sizeof(context_t));
  printk("  -> kstack is at: 0x%x\n", p->kstack);

  char *sp = p->kstack + KSTACKSIZE;
  printk("  -> Top of kstack (sp) is at: 0x%x\n", sp);

  // --- THE CORRECTED STACK SETUP ---

  // 1. Allocate space for the *entire* context struct on the stack at once.
  sp -= sizeof(context_t);
  printk("  -> sp after context allocation is at: 0x%x\n", sp);

  // 2. The context pointer now points to this 20-byte block.
  p->context = (context_t *)sp;

  // 3. Zero out the registers.
  memset(p->context, 0, sizeof(context_t));

  // 4. Explicitly set the 'eip' field.
  p->context->eip = (uint32_t)kernel_thread_test;

  p->state = RUNNABLE;

  printk("  -> Final context pointer p->context is: 0x%x\n", p->context);
  printk("  -> Value of p->context->eip is: 0x%x\n", p->context->eip);
  printk("--- End of process creation ---\n");
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

  context_t *dummy_context;
  swtch(&dummy_context, cpus[0].scheduler);

  printk("kmain: ERROR! Returned from scheduler swtch!\n");
  __builtin_unreachable();
}
