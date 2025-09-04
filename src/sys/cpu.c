#include "sys/cpu.h"
#include "debug/debug.h"
#include "defs.h"
#include "drivers/printk.h"
#include "sys/process.h"
#include "sys/tasks.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

extern proc_t ptable[MAX_PROCS];

void swtch(context_t **, context_t *);

// Stays 1 for now, since multithreading is not supported (YET)
int32_t ncpu = 1;
cpu_t cpus[NCPU];

// FIFO
void scheduler(void) {
  proc_t *p = NULL;
  cpu_t *c = &cpus[0];

  while (true) {
    run_scheduled_tasks();
    __asm__ volatile("sti");

    for (p = ptable; p < &ptable[MAX_PROCS]; p += 1) {
      if (p->state != RUNNABLE) {
        continue;
      }

      c->proc = p;
      p->state = RUNNING;

      swtch(&c->scheduler, p->context);
      BOCHS_MAGICBREAK();

      c->proc = NULL;
    }

    __asm__ volatile("hlt");
  }
}

void yield(void) {
  proc_t *p = myproc();

  p->state = RUNNABLE;                   // Mark self as runnable
  swtch(&p->context, cpus[0].scheduler); // Switch to the scheduler
}

void cpu_init(void) {
  for (int32_t i = 0; i < ncpu; i += 1) {
    cpus[i].scheduler = &cpus[i].scheduler_context;
    memset(cpus[i].scheduler, 0, sizeof(context_t));
    cpus[i].scheduler->eip = (uint32_t)scheduler;

    cpus[i].proc = NULL;
    cpus[i].ncli = 0;
    cpus[i].intena = 0;

    printk("[ OK ] Core %d initialised.\n", i);
  }
}
