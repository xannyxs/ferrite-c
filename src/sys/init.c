#include "drivers/printk.h"
#include "lib/stdlib.h"
#include "lib/string.h"
#include "memory/consts.h"
#include "memory/kmalloc.h"
#include "sys/process.h"

#include <stdbool.h>

extern proc_t ptables[NUM_PROC];
extern uint32_t pid_counter;
extern uint32_t page_directory[1024];

void init_process(void) {
  const proc_t *current_proc = myproc();

  if (current_proc->pid != 1) {
    abort("Init process should be PID 1!");
  }

  printk("Initial process started...!\n");

  pid_t pid = do_fork("shell");
  if (pid < 0) {
    abort("Init: could not create a new process");
  } else if (pid == 0) {
    shell_process();
    __builtin_unreachable();
  }

  printk("Init: Created child PID %d with PID %d\n", pid, current_proc->pid);

  while (true) {
    for (int32_t i = 0; i < NUM_PROC; i++) {
      proc_t *p = &ptables[i];
      if (p->state == ZOMBIE && p->parent == current_proc) {
        p->state = UNUSED;
        kfree(p->kstack);
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
