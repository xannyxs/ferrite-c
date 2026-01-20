#ifndef MODULE_TIMER_H
#define MODULE_TIMER_H

#include <types.h>

typedef void (*timer_callback_t)(unsigned long);

int register_timer_callback(timer_callback_t);

int unregister_timer_callback(timer_callback_t);

void trigger_timer_callbacks(unsigned long);

#endif
