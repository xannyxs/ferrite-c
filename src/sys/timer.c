#include "sys/timer.h"
#include "arch/x86/io.h"
#include "arch/x86/pit.h"
#include "drivers/printk.h"
#include "sys/process.h"

#define NUM_TIMERS 16

extern context_t* scheduler_context;
extern unsigned long long volatile ticks;
extern proc_t* current_proc;
static timer_t timers[NUM_TIMERS];

void wake_up_process(void* data)
{
    proc_t* p = (proc_t*)data;
    p->state = READY;
}

s32 add_timer(timer_t* timer)
{
    for (s32 i = 0; i < NUM_TIMERS; i++) {
        if (!timers[i].function) {
            timers[i] = *timer;
            return 0;
        }
    }

    return -1;
}

void check_timers(void)
{
    for (s32 i = 0; i < NUM_TIMERS; i += 1) {
        if (timers[i].function && ticks >= timers[i].expires) {
            timers[i].function(timers[i].data);
            timers[i].function = NULL;
        }
    }
}

/*
 * Sleep for specified seconds by blocking current process.
 * Current implementation: Direct timer callback wakes process
 *
 * POSIX approach would use: alarm(seconds) + pause() + SIGALRM handler
 * TODO: Implement SIGALRM-based sleep for full POSIX compliance
 */
s32 ksleep(s32 seconds)
{
    timer_t timer;

    timer.expires = ticks + (unsigned long long)seconds * HZ;
    timer.function = wake_up_process;
    timer.data = (void*)current_proc;

    s32 r = add_timer(&timer);
    if (r < 0) {
        printk("sleep: Timer array is full");
        return -1;
    }

    current_proc->state = SLEEPING;
    swtch(&current_proc->context, scheduler_context);

    return 0;
}

void sleeppid(void* channel)
{
    proc_t* p = myproc();
    cli();

    p->channel = channel;
    p->state = SLEEPING;

    swtch(&p->context, scheduler_context);

    p->channel = NULL;
    sti();
}
