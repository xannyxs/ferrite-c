#ifndef TASKS_H
#define TASKS_H

#include "arch/x86/idt/idt.h"

typedef void (*interrupt_callback_t)(struct interrupt_frame *);

void run_scheduled_tasks(void);

void schedule_task(interrupt_callback_t task);

#endif /* TASKS_H */
