#ifndef TIMER_H
#define TIMER_H

#include "types.h"

typedef struct timer {
    struct timer* next;
    struct timer* prev;

    unsigned long expires;
    void* data;
    void (*function)(void*);
} timer_t;

s32 sleep(s32 seconds);

void sleeppid(void* channel);

void check_timers(void);

#endif /* TIMER_H */
