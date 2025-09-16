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
extern void swtch(context_t **old, context_t *new);

proc_t *current_proc = NULL;
context_t *scheduler_context;

proc_t *myproc(void) { return current_proc; }

void yield(void) {
  proc_t *p = myproc();

  p->state = READY;
  swtch(&p->context, scheduler_context);
}

void init_kernel_thread(void) {
#if defined(__DEBUG)
  printk("First kernel thread is running!\n");
#endif

  pid_t pid = fork();
  if (pid < 0) {
    abort("Error");
  } else if (pid > 0) {
    printk("I am PID %d! I am a child\n", pid);

    int counter = 0;
    while (true) {
      counter++;
      if (counter % 10000000 == 0) {
        printk("Init thread: counter = %d\n", counter);
        yield();
      }
    }
  } else {
    printk("I am PID %d! I am a parent\n", pid);
  }

  yield();
}

void create_first_process(void) {
  proc_t *init = &ptables[0];
  memset(init, 0, sizeof(proc_t));

  init->state = EMBRYO;
  init->parent = NULL;

  init->pid = pid_counter;
  pid_counter++;

#ifndef __DEBUG
  memcpy(init->name, "init", 5);
#endif

  init->kstack = kmalloc(PAGE_SIZE);
  if (!init->kstack) {
    abort("create_first_process: initial process could not create a stack");
  }

  init->pgdir = page_directory;

  uint32_t *sp = (uint32_t *)((char *)init->kstack + PAGE_SIZE);
  *--sp = (uint32_t)init_kernel_thread; // Function to execute
  *--sp = 0;                            // ebp
  *--sp = 0;                            // ebx
  *--sp = 0;                            // esi
  *--sp = 0;                            // edi
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
    run_scheduled_tasks();

    for (int32_t i = 0; i < NUM_PROC; i += 1) {
      if (ptables[i].state != READY) {
        continue;
      }
      proc_t *p = &ptables[i];

      printk("Scheduler: switching to process %d\n", p->pid);

      p->state = RUNNING;
      current_proc = p;

      swtch(&scheduler_context, p->context);

      printk("Scheduler: back from process %d\n", p->pid);
      current_proc = NULL;
    }

    __asm__ volatile("sti");
    __asm__ volatile("hlt");
  }
}
