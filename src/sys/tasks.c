#include "sys/tasks.h"
#include "lib/stdlib.h"
#include <stdint.h>

#define MAX_TASKS 32

static interrupt_callback_t task_queue[MAX_TASKS];
static int32_t task_count = 0;

void schedule_task(interrupt_callback_t task) {
  if (task_count >= MAX_TASKS) {
    abort("Task queue is full");
  }

  task_queue[task_count] = task;
  task_count += 1;
}

void run_scheduled_tasks(void) {
  while (task_count > 0) {
    __asm__ volatile("cli");
    interrupt_callback_t task = task_queue[0];

    for (int32_t i = 0; i < task_count - 1; i++) {
      task_queue[i] = task_queue[i + 1];
    }
    task_count -= 1;

    __asm__ volatile("sti");
    task(NULL);
  }
}
