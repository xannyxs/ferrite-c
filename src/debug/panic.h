#ifndef PANIC_H
#define PANIC_H

#include "arch/x86/idt/idt.h"

__attribute__((__noreturn__)) void panic(registers_t *, const char *);

#endif /* PANIC_H */
