#include "sys/process.h"
#include "drivers/printk.h"
#include "memory/consts.h"
#include "memory/kmalloc.h"

#include <stddef.h>
#include <stdint.h>

extern uint32_t page_directory[1024];
extern proc_t *current_proc;

proc_t ptables[NUM_PROC];
uint32_t pid_counter = 1;

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

void process_a(void) {
  printk("Hello I am PID %d\n", current_proc->pid);

  for (;;) {
    yield();
  }
}

pid_t fork(void) {
  proc_t *p = alloc_proc();
  if (!p) {
    return -1;
  }

  p->kstack = kmalloc(PAGE_SIZE);
  if (!p->kstack) {
    p->state = UNUSED;
    return -1;
  }

  uint32_t *sp = (uint32_t *)((char *)p->kstack + PAGE_SIZE);
  *--sp = (uint32_t)process_a; // Function to execute
  *--sp = 0;                   // ebp
  *--sp = 0;                   // ebx
  *--sp = 0;                   // esi
  *--sp = 0;                   // edi

  p->context = (context_t *)sp;
  p->pgdir = page_directory;
  p->parent = current_proc;

  p->state = READY;
  return p->pid;
}

void init_ptables(void) {
  for (int32_t i = 0; i < NUM_PROC; i += 1) {
    ptables[i].state = UNUSED;
  }
}
