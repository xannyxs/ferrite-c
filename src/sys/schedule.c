#include "drivers/console.h"
#include "drivers/keyboard.h"
#include "drivers/printk.h"
#include "lib/stdlib.h"
#include "lib/string.h"
#include "memory/consts.h"
#include "memory/kmalloc.h"
#include "memory/vmm.h"
#include "sys/process.h"
#include "sys/tasks.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern uint32_t page_directory[1024];
extern uint32_t pid_counter;
extern proc_t ptables[NUM_PROC];

int32_t ticks_remaining;
context_t *scheduler_context;
proc_t *current_proc = NULL;
volatile bool need_resched = false;

inline proc_t *myproc(void) { return current_proc; }

inline void yield(void) {
  proc_t *p = myproc();
  if (!p) {
    return;
  }

  need_resched = false;
  p->state = READY;
  swtch(&p->context, scheduler_context);
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

void process_a(void) {
  uint32_t counter = 0;
  for (;;) {
    counter++;
    if (counter % 100000000 == 0) { // Print occasionally, not every loop!
      printk("Process A: %d\n", counter);
    }
  }
}

void shell_init(void) {
  console_init();

  pid_t pid = fork("process_a", process_a);
  if (pid < 0) {
    abort("Init: could not create a new process");
  }
  for (;;) {
    run_scheduled_tasks();

    // yield();
  }
}

void init_process(void) {
  if (current_proc->pid != 1) {
    abort("Init process should be PID 1!");
  }

  printk("Initial process started...!\n");

  pid_t pid = fork("shell", shell_init);
  if (pid < 0) {
    abort("Init: could not create a new process");
  }

  printk("Init: Created child PID %d with PID %d\n", pid, current_proc->pid);

  while (true) {
    for (int32_t i = 0; i < NUM_PROC; i++) {
      proc_t *p = &ptables[i];
      if (p->state == ZOMBIE && p->parent == current_proc) {
        p->state = UNUSED;
        printk("Init: reaped zombie PID %d\n", p->pid);
      }
    }

    yield();
  }
}

void create_initial_process(void) {
  proc_t *init = &ptables[0];
  memset(init, 0, sizeof(proc_t));

  init->state = EMBRYO;
  init->parent = NULL;

  init->pid = pid_counter;
  pid_counter++;

  memcpy(init->name, "init", 5);
  init->kstack = kmalloc(PAGE_SIZE);
  if (!init->kstack) {
    abort("create_first_process: initial process could not create a stack");
  }

  init->pgdir = page_directory;

  uint32_t *sp = (uint32_t *)((char *)init->kstack + PAGE_SIZE);
  *--sp = (uint32_t)init_process; // Function to execute
  *--sp = 0;                      // ebp
  *--sp = 0;                      // ebx
  *--sp = 0;                      // esi
  *--sp = 0;                      // edi
  init->context = (context_t *)sp;

  init->state = READY;
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

      // printk("Scheduler: switching to process %d\n", p->pid);

      p->state = RUNNING;
      current_proc = p;
      ticks_remaining = TIME_QUANTUM;

      swtch(&scheduler_context, p->context);

      // printk("Scheduler: back from process %d\n", p->pid);
      current_proc = NULL;
    }

    __asm__ volatile("sti");
    __asm__ volatile("hlt");
  }
}
