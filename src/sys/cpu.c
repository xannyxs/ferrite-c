#include "sys/cpu.h"
#include "debug/debug.h"
#include "defs.h"
#include "drivers/printk.h"
#include "lib/stdlib.h"
#include "memory/kmalloc.h"
#include "memory/vmm.h"
#include "sys/process.h"
#include "sys/tasks.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

extern proc_t ptable[MAX_PROCS];
extern void swtch(context_t **, context_t *);

// Stays 1 for now, since multithreading is not supported (YET)
int32_t ncpu = 1;
cpu_t cpus[NCPU];

// FIFO
void scheduler(void) {
  proc_t *p = NULL;
  cpu_t *c = &cpus[0];

  while (true) {
    // The entire process-finding loop should be atomic.
    // We assume interrupts are disabled upon entering this loop.

    for (p = ptable; p < &ptable[MAX_PROCS]; p += 1) {
      if (p->state != RUNNABLE) {
        continue;
      }

      c->proc = p;
      p->state = RUNNING;

      // The switch itself happens with interrupts OFF.
      // The process's context will resume after its own swtch,
      // and it is responsible for re-enabling interrupts.
      swtch(&c->scheduler, p->context);

      // We return here from a process yielding, with interrupts disabled.
      c->proc = NULL;
    }

    // If we found no processes to run, idle safely.
    __asm__ volatile("sti");
    __asm__ volatile("hlt");
    // The interrupt handler that woke us up will have run, and then
    // execution resumes here. We disable interrupts before looping again.
    __asm__ volatile("cli");
  }
}

void yield(void) {
  proc_t *p = myproc();
  uint32_t current_esp;

  __asm__ volatile("mov %%esp, %0" : "=r"(current_esp));
  printk("yield: esp=0x%x, kstack_bottom=0x%x, kstack_top=0x%x\n", current_esp,
         (uint32_t)p->kstack, (uint32_t)p->kstack + KSTACKSIZE);

  if (current_esp < (uint32_t)p->kstack ||
      current_esp > (uint32_t)p->kstack + KSTACKSIZE) {
    printk("FATAL: Stack pointer is outside of its allocated bounds!\n");
    for (;;)
      ;
  }

  __asm__ volatile("cli");
  p->state = RUNNABLE;
  swtch(&p->context, cpus[0].scheduler);
  __asm__ volatile("sti");
}

void cpu_init(void) {
  int32_t i = 0;
  void *scheduler_kstack = kalloc(KSTACKSIZE * 2);
  if (!scheduler_kstack) {
    abort("Fatal: Could not allocate scheduler kstack");
  }

  char *sp = (char *)scheduler_kstack + KSTACKSIZE;
  sp -= sizeof(context_t);
  cpus[i].scheduler = (context_t *)sp;
  memset(cpus[i].scheduler, 0, sizeof(context_t));

  cpus[i].scheduler->eip = (uint32_t)scheduler;
  cpus[i].proc = NULL;
  cpus[i].ncli = 0;
  cpus[i].intena = 0;
}
