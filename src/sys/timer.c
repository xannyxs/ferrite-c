#include "sys/timer.h"
#include "arch/x86/pit.h"
#include "debug/panic.h"
#include "sys/process.h"

#include <stddef.h>
#include <stdint.h>

#define NUM_TIMERS 16

extern volatile uint64_t ticks;
extern proc_t *current_proc;
static timer_t timers[NUM_TIMERS];

void wake_up_process(void *data) {
  proc_t *p = (proc_t *)data;
  p->state = READY;
}

void add_timer(timer_t *timer) {
  for (int32_t i = 0; i < NUM_TIMERS; i++) {
    if (!timers[i].function) {
      timers[i] = *timer;
      return;
    }
  }

  panic("No free timer slots");
}

void check_timers(void) {
  for (int32_t i = 0; i < NUM_TIMERS; i += 1) {
    if (timers[i].function && ticks >= timers[i].expires) {
      timers[i].function(timers[i].data);
      timers[i].function = NULL;
    }
  }
}

int32_t sleep(int32_t seconds) {
  timer_t timer;

  timer.expires = ticks + seconds * HZ;
  timer.function = wake_up_process;
  timer.data = (void *)current_proc;

  add_timer(&timer);

  current_proc->state = SLEEPING;
  schedule();

  return 0;
}
