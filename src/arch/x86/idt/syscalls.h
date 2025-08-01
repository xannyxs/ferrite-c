#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "arch/x86/idt/idt.h"

#include <stddef.h>

enum syscalls_e { EXIT, WRITE, READ, SYSCALL_COUNT };

void syscall_dispatcher_c(registers_t *);

#endif /* SYSCALLS_H */
