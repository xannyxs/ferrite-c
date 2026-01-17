#ifndef PANIC_H
#define PANIC_H

#include "arch/x86/idt/idt.h"

__attribute__((__noreturn__)) void panic(trapframe_t*, char const*);

#endif /* PANIC_H */
