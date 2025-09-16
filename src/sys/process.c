#include "sys/process.h"
#include "drivers/console.h"
#include "drivers/printk.h"
#include "memory/consts.h"
#include "memory/kmalloc.h"
#include "memory/memory.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

extern uint32_t page_directory[1024];
extern proc_t *current_proc;

proc_t ptables[NUM_PROC];
uint32_t pid_counter = 1;

void process_list(void) {
  printk("PID  STATE  NAME\n");
  printk("---  -----  ----\n");

  for (int32_t i = 0; i < NUM_PROC; i += 1) {
    if (ptables[i].state != UNUSED) {
      const char *state_str[] = {"UNUSED", "EMBRYO",  "SLEEPING",
                                 "READY",  "RUNNING", "ZOMBIE"};

      printk("%d  %s  %s\n", ptables[i].pid, state_str[ptables[i].state],
             ptables[i].name);
    }
  }
}

proc_t *alloc_proc(void) {
  for (int32_t i = 0; i < NUM_PROC; i += 1) {
    if (ptables[i].state == UNUSED) {
      ptables[i].state = EMBRYO;
      ptables[i].pid = pid_counter;
      pid_counter++;

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

pid_t fork(char *name, void (*f)(void)) {
  proc_t *p = alloc_proc();
  if (!p) {
    return -1;
  }

  p->kstack = kmalloc(PAGE_SIZE);
  if (!p->kstack) {
    p->state = UNUSED;
    return -1;
  }

  uint32_t *sp =
      (uint32_t *)((char *)p->kstack + PAGE_SIZE - sizeof(block_header_t));
  *--sp = 0;
  *--sp = (uint32_t)f;
  *--sp = 0; // ebp
  *--sp = 0; // ebx
  *--sp = 0; // esi
  *--sp = 0; // edi

  p->context = (context_t *)sp;
  p->pgdir = page_directory;
  p->parent = current_proc;
  memcpy(p->name, name, strlen(name)); // FIXME: Take care for overflows

  p->state = READY;
  return p->pid;
}

void init_ptables(void) {
  for (int32_t i = 0; i < NUM_PROC; i += 1) {
    ptables[i].state = UNUSED;
  }
}
