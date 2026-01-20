#ifndef MODULE_KEYBOARD_H
#define MODULE_KEYBOARD_H

#include <types.h>

typedef void (*keyboard_callback_t)(u8 scancode, int pressed);

int register_keyboard_callback(keyboard_callback_t callback);

int unregister_keyboard_callback(keyboard_callback_t callback);

void trigger_keyboard_callbacks(u8 scancode, int pressed);

#endif
