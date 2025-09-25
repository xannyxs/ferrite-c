#include "sys/process.h"
#include "drivers/printk.h"
#include "lib/stdlib.h"
#include "lib/string.h"
#include "memory/consts.h"
#include "memory/kmalloc.h"
#include "memory/memory.h"
#include "memory/page.h"
#include "sys/signal/signal.h"
#include "sys/timer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern uint32_t page_directory[1024];

proc_t ptables[NUM_PROC];
proc_t *current_proc = NULL;
context_t *scheduler_context;
uint32_t pid_counter = 1;

int32_t ticks_remaining;
volatile bool need_resched = false;

/* Private */

static proc_t *alloc_proc(void) {
  for (int32_t i = 0; i < NUM_PROC; i += 1) {
    if (ptables[i].state == UNUSED) {
      ptables[i].state = EMBRYO;

      return &ptables[i];
    }
  }

  return NULL;
}

void fork_return(void) {
  uint32_t retval;
  __asm__ volatile("popl %0" : "=r"(retval));
  __asm__ volatile("movl %0, %%eax\n"
                   "ret"
                   :
                   : "r"(retval)
                   : "eax");
}

/* Public */

inline proc_t *myproc(void) { return current_proc; }

inline proc_t *find_process(pid_t pid) {
  for (int32_t i = 0; i < NUM_PROC; i += 1) {
    if (ptables[i].pid == pid) {
      return &ptables[i];
    }
  }

  return NULL;
}

void init_ptables(void) {
  for (int32_t i = 0; i < NUM_PROC; i += 1) {
    ptables[i].state = UNUSED;
  }
}

inline void check_resched(void) {
  proc_t *p = myproc();
  if (!p || need_resched == false) {
    return;
  }

  printk("Preempting PID %d\n", current_proc->pid);
  need_resched = false;
  yield();
}

__attribute__((naked)) void fork_ret(void) {
  __asm__ volatile("movl $0, %%eax\n\t"
                   "popl %%ecx\n\t"
                   "jmp *%%ecx" ::
                       : "eax", "ecx");
}

// WIP
int32_t do_wait(void) {
  proc_t *p = NULL;
  while (true) {
    for (int32_t i = 0; i < NUM_PROC; i += 1) {
      p = &ptables[i];

      if (p == current_proc) {
        continue;
      }
      if (p->state == ZOMBIE) {
        p->state = UNUSED;
        return p->pid;
      }
    }

    sleep(10);
  }
}

pid_t do_exec(const char *name, void (*f)(void)) {
  proc_t *p = alloc_proc();
  if (!p) {
    return -1;
  }

  *p = *current_proc;

  p->kstack = get_free_page();
  if (!p->kstack) {
    p->state = UNUSED;
    return -1;
  }

  uint32_t *ctx = (uint32_t *)((char *)p->kstack + PAGE_SIZE);

  *(--ctx) = (uint32_t)f; // EIP
  *(--ctx) = 0;           // EBP
  *(--ctx) = 0;           // EBX
  *(--ctx) = 0;           // ESI
  *(--ctx) = 0;           // EDI

  p->pid = pid_counter;
  pid_counter += 1;
  p->context = (context_t *)ctx;
  p->pgdir = page_directory;
  p->parent = current_proc;
  strlcpy(p->name, name, sizeof(p->name));

  p->state = READY;
  return p->pid;
}

pid_t do_fork(const char *name) {
  proc_t *p = alloc_proc();
  if (!p) {
    return -1;
  }

  *p = *current_proc;

  p->kstack = get_free_page();
  if (!p->kstack) {
    p->state = UNUSED;
    return -1;
  }

  uint32_t *ctx = (uint32_t *)((char *)p->kstack + PAGE_SIZE);

  uint32_t caller_return;
  __asm__ volatile("movl 4(%%ebp), %0" : "=r"(caller_return));

  *(--ctx) = caller_return;      // Return address for fork_ret (on stack)
  *(--ctx) = (uint32_t)fork_ret; // EIP
  *(--ctx) = 0;                  // EBP
  *(--ctx) = 0;                  // EBX
  *(--ctx) = 0;                  // ESI
  *(--ctx) = 0;                  // EDI

  p->pid = pid_counter;
  pid_counter += 1;
  p->context = (context_t *)ctx;
  p->pgdir = page_directory;
  p->parent = current_proc;
  strlcpy(p->name, name, sizeof(p->name));

  p->state = READY;
  return p->pid;
}

inline void yield(void) {
  proc_t *p = myproc();
  if (!p) {
    return;
  }

  need_resched = false;
  p->state = READY;
  swtch(&p->context, scheduler_context);
}

void schedule(void) {
  static char *scheduler_stack = NULL;
  if (!scheduler_stack) {
    scheduler_stack = kmalloc(PAGE_SIZE);
    if (!scheduler_stack) {
      abort("Cannot allocate scheduler stack");
    }
    scheduler_context = (context_t *)(scheduler_stack + PAGE_SIZE - 64);
  }

  // FIFO
  while (true) {
    for (int32_t i = 0; i < NUM_PROC; i += 1) {
      if (ptables[i].state != READY) {
        continue;
      }
      proc_t *p = &ptables[i];
      current_proc = p;

      // printk("Scheduler: switching to process %d\n", p->pid);

      handle_signal();
      if (p->state != READY && p->state != RUNNING) {
        current_proc = NULL;
        continue;
      }

      p->state = RUNNING;
      ticks_remaining = TIME_QUANTUM;

      swtch(&scheduler_context, p->context);

      if (current_proc && current_proc->state == RUNNING) {
        current_proc->state = READY;
      }

      // printk("Scheduler: back from process %d\n", p->pid);
      current_proc = NULL;
    }

    sti();
    __asm__ volatile("hlt");
  }
}
