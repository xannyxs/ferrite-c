#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "arch/x86/idt/idt.h"

#define SYSCALL_ATTR \
    __attribute__((target("general-regs-only"), warn_unused_result))

enum socket_subcalls_e {
    SYS_SOCKET = 1,
    SYS_BIND = 2,
    SYS_CONNECT = 3,
    SYS_LISTEN = 4,
    SYS_ACCEPT = 5,
};

enum syscalls_e {
    SYS_EXIT = 1,
    SYS_FORK = 2,
    SYS_READ = 3,
    SYS_WRITE = 4,
    SYS_OPEN = 5,
    SYS_CLOSE = 6,
    SYS_WAITPID = 7,
    SYS_UNLINK = 10,
    SYS_CHDIR = 12,
    SYS_TIME = 13,
    SYS_STAT = 18,
    SYS_LSEEK = 19,
    SYS_GETPID = 20,
    SYS_SETUID = 23,
    SYS_GETUID = 24,
    SYS_KILL = 37,
    SYS_MKDIR = 39,
    SYS_RMDIR = 40,
    SYS_SIGNAL = 48,
    SYS_GETEUID = 49,
    SYS_READDIR = 89,
    SYS_TRUNCATE = 92,
    SYS_FTRUNCATE = 93,
    SYS_SOCKETCALL = 102,
    SYS_FCHDIR = 133,
    SYS_NANOSLEEP = 162,
};

void syscall_dispatcher_c(registers_t*);

int sys_socketcall(int call, unsigned long* args);

#endif /* SYSCALLS_H */
