#ifndef TIMER_H
#define TIMER_H

#include <types.h>

typedef struct timer {
    struct timer* next;
    struct timer* prev;

    unsigned long long expires;
    void* data;
    void (*function)(void*);
} timer_t;

int ksleep(int);

int knanosleep(u32);

void waitchan(void*);

void check_timers(void);

#endif /* TIMER_H */
