#ifndef TIMER_H
#define TIMER_H

#include "types.h"

typedef struct timer {
    struct timer* next;
    struct timer* prev;

    unsigned long long expires;
    void* data;
    void (*function)(void*);
} timer_t;

s32 ksleep(s32 seconds);

s32 knanosleep(s32 ms);

void waitchan(void* channel);

void check_timers(void);

#endif /* TIMER_H */
