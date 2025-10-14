#include "arch/x86/idt/syscalls.h"
#include "arch/x86/idt/idt.h"
#include "arch/x86/time/time.h"
#include "drivers/printk.h"
#include "lib/string.h"
#include "net/socket.h"
#include "sys/file/file.h"
#include "sys/file/inode.h"
#include "sys/file/stat.h"
#include "sys/process/process.h"
#include "sys/signal/signal.h"
#include "sys/timer/timer.h"
#include "syscalls.h"
#include "types.h"

#include <stdbool.h>

__attribute__((target("general-regs-only"))) static void sys_exit(s32 status)
{
    do_exit(status);
}

// // TODO: Make _read function
// __attribute__((target("general-regs-only"), warn_unused_result)) static s32
// sys_read(s32 fd, void* buf, size_t count)
// {
//     (void)fd;
//     (void)buf;
//     (void)count;
//
//     printk("Read\n");
//     return 0;
// }
//
// // TODO: Make _write function
// __attribute__((target("general-regs-only"), warn_unused_result)) static s32
// sys_write(s32 fd, void* buf, size_t count)
// {
//     (void)fd;
//     (void)buf;
//     (void)count;
//
//     printk("Write\n");
//     return 0;
// }

SYSCALL_ATTR static s32 sys_fork(void)
{
    return do_fork("user process");
}

SYSCALL_ATTR static pid_t sys_waitpid(pid_t pid, s32* status, s32 options)
{
    (void)pid;
    (void)options;
    return do_wait(status);
}

SYSCALL_ATTR static time_t sys_time(time_t* tloc)
{
    time_t current_time = getepoch();

    if (tloc != NULL) {
        *tloc = current_time;
    }

    return current_time;
}

SYSCALL_ATTR static pid_t sys_getpid(void)
{
    return myproc()->pid;
}

SYSCALL_ATTR static uid_t sys_getuid(void)
{
    return getuid();
}

SYSCALL_ATTR static s32 sys_kill(pid_t pid, s32 sig)
{
    proc_t* caller = myproc();
    proc_t* target = find_process(pid);
    if (!target || !caller) {
        return -1;
    }

    if (caller->euid == ROOT_UID) {
        return do_kill(pid, sig);
    }

    if (caller->euid == target->euid || caller->uid == target->uid) {
        return do_kill(pid, sig);
    }

    return -1;
}

SYSCALL_ATTR static uid_t sys_geteuid(void)
{
    return geteuid();
}

SYSCALL_ATTR static s32 sys_setuid(uid_t uid)
{
    proc_t* p = myproc();

    if (p->euid == 0) {
        p->uid = uid;
        p->euid = uid;

        return 0;
    }

    if (p->uid == uid || p->euid == uid) {
        p->euid = uid;

        return 0;
    }

    return -1;
}

SYSCALL_ATTR static s32 sys_nanosleep(void)
{
    return knanosleep(1000);
}

// TODO: Should not just work on sockets
SYSCALL_ATTR static s32 sys_close(s32 fd)
{
    file_t* f = getfd(fd);
    if (!f) {
        return -1;
    }

    if (S_ISSOCK(f->f_inode->i_mode) == true) {
        return socket_close(f);
    }

    return -1;
}

__attribute__((target("general-regs-only"))) void
syscall_dispatcher_c(registers_t* reg)
{
    switch (reg->eax) {
    case SYS_EXIT:
        sys_exit((s32)reg->ebx);
        break;

    case SYS_FORK:
        reg->eax = sys_fork();
        break;

    case SYS_CLOSE:
        reg->eax = sys_close((s32)reg->ebx);
        break;

    case SYS_WAITPID:
        reg->eax = sys_waitpid((s32)reg->ebx, (s32*)reg->ecx, (s32)reg->edx);
        break;

    case SYS_TIME:
        reg->eax = sys_time((time_t*)reg->ebx);
        break;

    case SYS_GETPID:
        reg->eax = sys_getpid();
        break;

    case SYS_GETUID:
        reg->eax = sys_getuid();
        break;

    case SYS_KILL:
        reg->eax = sys_kill((s32)reg->ebx, (s32)reg->ecx);
        break;

    case SYS_SOCKETCALL:
        reg->eax = sys_socketcall((s32)reg->ebx, (unsigned long*)reg->ecx);
        break;

    case SYS_SIGNAL:
        break;

    case SYS_GETEUID:
        reg->eax = sys_geteuid();
        break;

    case SYS_SETUID:
        reg->eax = sys_setuid((s32)reg->ebx);
        break;

    case SYS_NANOSLEEP:
        reg->eax = sys_nanosleep();
        break;

    default:
        printk("Nothing...?\n");
        break;
    }

    check_resched();
}
