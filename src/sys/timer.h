#ifndef TIMER_H
#define TIMER_H

typedef struct timer {
  struct timer *next;
  struct timer *prev;

  unsigned long expires;
  void *data;
  void (*function)(void *);
} timer_t;

#endif /* TIMER_H */
