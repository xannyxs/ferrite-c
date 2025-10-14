#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "arch/x86/idt/idt.h"

enum syscalls_e {
    SYS_READ = 0,
    SYS_WRITE = 1,
    SYS_OPEN = 5,
    SYS_CLOSE = 6,
    SYS_WAITPID = 7,
    SYS_CREAT = 8,
    SYS_LINK = 9,
    SYS_UNLINK = 10,
    SYS_EXECVE = 11,
    SYS_CHDIR = 12,
    SYS_TIME = 13,
    SYS_SETUID = 23,
    SYS_GETUID = 24,
    SYS_ACCESS = 33,
    SYS_NANOSLEEP = 35,
    SYS_GETPID = 39,
    SYS_SOCKET = 41,
    SYS_CONNECT = 42,
    SYS_BRK = 45,
    SYS_SIGNAL = 48,
    SYS_BIND = 49,
    SYS_LISTEN = 50,
    SYS_FORK = 57,
    SYS_EXIT = 60,
    SYS_KILL = 62,
    SYS_GETEUID = 107,
};

void syscall_dispatcher_c(registers_t*);

#endif /* SYSCALLS_H */
