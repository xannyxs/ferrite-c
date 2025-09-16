#include "sys/process.h"

#include <stddef.h>
#include <stdint.h>

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

void init_ptables(void) {
  for (int32_t i = 0; i < NUM_PROC; i += 1) {
    ptables[i].state = UNUSED;
  }
}
