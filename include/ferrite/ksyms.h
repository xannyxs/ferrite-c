#ifndef _KSYMS_H
#define _KSYMS_H

#include "drivers/printk.h"
#include "memory/kmalloc.h"
#include "module/keyboard.h"
#include "module/timer.h"

#include <ferrite/module.h>
#include <ferrite/string.h>

#define EXPORT_SYM(sym) { #sym, (unsigned long)sym }

struct symbol_table {
    char const* name;
    unsigned long addr;
} symbols[] = { EXPORT_SYM(printk),
                EXPORT_SYM(kmalloc),
                EXPORT_SYM(kfree),
                EXPORT_SYM(register_keyboard_callback),
                EXPORT_SYM(unregister_keyboard_callback),
                EXPORT_SYM(register_timer_callback),
                EXPORT_SYM(unregister_timer_callback) };

#undef EXPORT_SYM

unsigned long ksym_lookup(char const* name)
{
    for (int i = 0; symbols[i].name != NULL; i++) {
        if (strcmp(symbols[i].name, name) == 0) {
            return symbols[i].addr;
        }
    }

    return 0;
}

#endif /* KSYMS_H */
