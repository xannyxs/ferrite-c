#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "arch/x86/idt/idt.h"

enum syscalls_e {
    SYS_EXIT = 1,
    SYS_FORK = 2,
    SYS_READ = 3,
    SYS_WRITE = 4,
    SYS_OPEN = 5,
    SYS_CLOSE = 6,
    SYS_WAITPID = 7,
    SYS_CREAT = 8,
    SYS_LINK = 9,
    SYS_UNLINK = 10,
    SYS_EXECVE = 11,
    SYS_CHDIR = 12,
    SYS_TIME = 13,
    SYS_GETPID = 20,
    SYS_SETUID = 23,
    SYS_GETUID = 24,
    SYS_ACCESS = 33,
    SYS_KILL = 37,
    SYS_DUP = 41,
    SYS_BRK = 45,
    SYS_SIGNAL = 48,
    SYS_GETEUID = 49,
    SYS_NANOSLEEP = 162,
};

void syscall_dispatcher_c(registers_t*);

#endif /* SYSCALLS_H */
