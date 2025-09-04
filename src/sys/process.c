#include "sys/process.h"
#include "defs.h"
#include "drivers/printk.h"
#include "lib/stdlib.h"
#include "memory/consts.h"
#include "memory/kmalloc.h"
#include "sys/cpu.h"

#include <stdint.h>
#include <string.h>

extern int32_t ncpu;
extern cpu_t cpus[];

extern void trapret(void);
extern void forkret(void);

static uint32_t pid_counter = 0;
proc_t ptable[MAX_PROCS];

/* Private */

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
proc_t *myproc(void) {
  __asm__ volatile("cli");
  cpu_t *c = &cpus[0];
  proc_t *p = c->proc;
  __asm__ volatile("sti");

  return p;
}

// TODO: Pre-Allocate procs
proc_t *proc_alloc(void) {
  for (int32_t i = 0; i < MAX_PROCS; i += 1) {
    if (ptable[i].state != UNUSED) {
      continue;
    }

    // Atomically claim the process slot to avoid a race condition.
    ptable[i].state = EMBRYO;
    return &ptable[i];
  }

  return NULL;
}

/* Public */

/*
 * @return On success, the PID of the child process is returned in the parent,
 * and 0 is returned in the child.  On failure, -1 is returned in the parent, no
 * child process is created, and errno is set to indicate the error.
 *
 * https://man7.org/linux/man-pages/man2/fork.2.html
 */
pid_t sys_fork(void) {
  proc_t *p = proc_alloc();
  if (!p) {
    return -1;
  }
  proc_t *curproc = myproc();

  p->kstack = kmalloc(PAGE_SIZE);
  if (!p->kstack) {
    // Keeping the process allocated to be reused to reduce the
    // CPU overhead of memory allocation.
    p->state = UNUSED;
    return -1;
  }

  char *sp = p->kstack + KSTACKSIZE;
  sp -= sizeof(*p->trap);
  p->trap = (trapframe_t *)sp;

  sp -= 4;
  *(uint32_t *)sp = (uint32_t)trapret;

  sp -= sizeof(*p->context);
  p->context = (context_t *)sp;
  memset(p->context, 0, sizeof(*p->context));
  p->context->eip = (uint32_t)forkret;

  *p->trap = *curproc->trap;
  p->memory_limit = curproc->memory_limit;
  p->parent = curproc;
  p->trap->eax = 0;
  p->pid = pid_counter;
  pid_counter += 1;

  p->state = RUNNABLE;

  return p->pid;
}

void proc_init(void) {
  for (int32_t i = 0; i < MAX_PROCS; i += 1) {
    ptable[i].state = UNUSED;
  }
}
