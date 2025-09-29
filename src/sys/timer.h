#ifndef TIMER_H
#define TIMER_H

#include "sys/process.h"
#include <stdint.h>

typedef struct timer {
  struct timer *next;
  struct timer *prev;

  unsigned long expires;
  void *data;
  void (*function)(void *);
} timer_t;

int32_t sleep(int32_t seconds);

void sleeppid(void *channel);

void check_timers(void);

#endif /* TIMER_H */
