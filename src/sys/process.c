#include "sys/process.h"
#include "arch/x86/memlayout.h"
#include "defs.h"
#include "drivers/printk.h"
#include "lib/stdlib.h"
#include "memory/consts.h"
#include "memory/kmalloc.h"
#include "memory/vmm.h"
#include "sys/cpu.h"

#include <stdint.h>
#include <string.h>

extern int32_t ncpu;
extern cpu_t cpus[];
extern uint32_t page_directory[1024];

extern void trapret(void);
extern void forkret(void);

static uint32_t pid_counter = 0;
proc_t ptable[MAX_PROCS];

/* Private */

uint32_t *copyuvm(uint32_t *pgdir, uint32_t sz) {
  uint32_t *d = kalloc(PAGE_SIZE);
  if (!d) {
    return NULL;
  }
  memset(d, 0, PAGE_SIZE);

  for (int32_t i = KERNBASE_PD_INDEX; i < 1024; i += 1) {
    d[i] = page_directory[i];
  }

  uint32_t *pte;
  for (uint32_t i = 0; i < sz; i += PAGE_SIZE) {
    pte = vmm_walkpgdir(pgdir, (void *)i, 0);
    if (!pte || !(*pte & PTE_P)) {
      // FIXME: This should be handled gracefully, not with abort.
      abort("copyuvm: parent page not present");
    }

    uint32_t pa = (*pte) & ~0xFFF;
    uint32_t flags = (*pte) & 0xFFF;

    void *mem = kalloc(PAGE_SIZE);
    if (!mem) {
      // FIXME: Gracefully free 'd' and any previously allocated pages.
      abort("copyuvm: out of memory");
    }

    memmove(mem, (char *)P2V_WO(pa), PAGE_SIZE);
    void *vmem = (void *)V2P_WO((uintptr_t)mem);
    int32_t r = vmm_map_page_dir(d, vmem, (void *)i, flags);
    if (r < 0) {
      kfree(mem);
      // FIXME: Gracefully free 'd'.
      abort("copyuvm: something went wrong with mapping");
    }
  }

  return d;
}

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

// void print_process(proc_t p) {}

/*
 * @return On success, the PID of the child process is returned in the parent,
 * and 0 is returned in the child.  On failure, -1 is returned in the parent,
 * no child process is created, and errno is set to indicate the error.
 *
 * https://man7.org/linux/man-pages/man2/fork.2.html
 */
pid_t sys_fork(void) {
  proc_t *p = proc_alloc();
  if (!p) {
    return -1;
  }
  proc_t *curproc = myproc();
  __asm__ volatile("sti");

  p->kstack = kalloc(KSTACKSIZE);
  if (!p->kstack) {
    p->state = UNUSED;
    return -1;
  }

  char *sp = p->kstack + KSTACKSIZE;
  sp -= sizeof(*p->trap);
  p->trap = (trapframe_t *)sp;
  sp -= sizeof(*p->context);
  p->context = (context_t *)sp;
  memset(p->context, 0, sizeof(*p->context));

  p->context->eip = (uint32_t)forkret;
  p->pgdir = copyuvm(curproc->pgdir, curproc->memory_limit);
  if (!p->pgdir) {
    kfree(p->kstack);
    p->kstack = NULL;
    p->state = UNUSED;
    return -1;
  }
  *p->trap = *curproc->trap;
  p->memory_limit = curproc->memory_limit;
  p->parent = curproc;
  p->trap->eax = 0;
  p->pid = pid_counter++;
  p->state = RUNNABLE;

  return p->pid;
}

void proc_init(void) {
  for (int32_t i = 0; i < MAX_PROCS; i += 1) {
    ptable[i].state = UNUSED;
  }
}
