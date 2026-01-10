#include "module/timer.h"
#include "memory/kmalloc.h"

#include <ferrite/errno.h>

typedef struct timer_listener {
    timer_callback_t callback;
    struct timer_listener* next;
} timer_listener_t;

static timer_listener_t* timer_cb = NULL;

int register_timer_callback(timer_callback_t callback)
{
    if (!callback) {
        return -EINVAL;
    }

    timer_listener_t* cb = kmalloc(sizeof(timer_listener_t));
    if (!cb) {
        return -ENOMEM;
    }

    cb->callback = callback;
    cb->next = timer_cb;
    timer_cb = cb;

    return 0;
}

int unregister_timer_callback(timer_callback_t callback)
{
    timer_listener_t** prev = &timer_cb;
    timer_listener_t* current = timer_cb;

    while (current) {
        if (current->callback == callback) {
            *prev = current->next;

            kfree(current);

            return 0;
        }

        prev = &current->next;
        current = current->next;
    }

    return -ENOENT;
}

void trigger_timer_callbacks(unsigned long ticks)
{
    timer_listener_t* tmp = timer_cb;

    while (tmp) {
        tmp->callback(ticks);

        tmp = tmp->next;
    }
}
