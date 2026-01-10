#include "module/keyboard.h"
#include "memory/kmalloc.h"

#include <ferrite/errno.h>

typedef struct keyboard_listener {
    keyboard_callback_t callback;
    struct keyboard_listener* next;
} keyboard_listener_t;

static keyboard_listener_t* keyboard_cb = NULL;

int register_keyboard_callback(keyboard_callback_t callback)
{
    if (!callback) {
        return -EINVAL;
    }

    keyboard_listener_t* cb = kmalloc(sizeof(keyboard_listener_t));
    if (!cb) {
        return -ENOMEM;
    }

    cb->callback = callback;
    cb->next = keyboard_cb;
    keyboard_cb = cb;

    return 0;
}

int unregister_keyboard_callback(keyboard_callback_t callback)
{
    keyboard_listener_t** prev = &keyboard_cb;
    keyboard_listener_t* current = keyboard_cb;

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

void trigger_keyboard_callbacks(u8 scancode, int pressed)
{
    keyboard_listener_t* tmp = keyboard_cb;

    while (tmp) {
        tmp->callback(scancode, pressed);

        tmp = tmp->next;
    }
}
