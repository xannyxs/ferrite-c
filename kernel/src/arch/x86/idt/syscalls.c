#include "arch/x86/idt/syscalls.h"
#include "arch/x86/idt/idt.h"
#include "arch/x86/time/time.h"
#include "fs/exec.h"
#include "sys/process/process.h"
#include "sys/signal/signal.h"
#include "sys/timer/timer.h"
#include "syscalls.h"

#include <ferrite/string.h>
#include <types.h>
#include <uapi/errno.h>

#define SYSCALL_ENTRY_0(num, fname) \
    [num] = { .handler = (void*)(sys_##fname), .nargs = 0, .name = #fname }

#define SYSCALL_ENTRY_1(num, fname) \
    [num] = { .handler = (void*)(sys_##fname), .nargs = 1, .name = #fname }

#define SYSCALL_ENTRY_2(num, fname) \
    [num] = { .handler = (void*)(sys_##fname), .nargs = 2, .name = #fname }

#define SYSCALL_ENTRY_3(num, fname) \
    [num] = { .handler = (void*)(sys_##fname), .nargs = 3, .name = #fname }

#define SYSCALL_ENTRY_5(num, fname) \
    [num] = { .handler = (void*)(sys_##fname), .nargs = 5, .name = #fname }

__attribute__((target("general-regs-only"))) static void sys_exit(s32 status)
{
    do_exit(status);
}

SYSCALL_ATTR static s32 sys_fork(trapframe_t* tf)
{
    return do_fork(tf, "user process");
}

SYSCALL_ATTR static pid_t sys_waitpid(pid_t pid, s32* status, s32 options)
{
    (void)pid;
    (void)options;

    return do_wait(status);
}

SYSCALL_ATTR static int sys_execve(
    char const* filename,
    char const* const* argv,
    char const* const* envp,
    trapframe_t* regs
)
{
    return do_execve(filename, argv, envp, regs);
}

SYSCALL_ATTR static time_t sys_time(time_t* tloc)
{
    time_t current_time = getepoch();
    if (tloc) {
        *tloc = current_time;
    }

    return current_time;
}

SYSCALL_ATTR static pid_t sys_getpid(void) { return myproc()->pid; }

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

SYSCALL_ATTR static s32 sys_nanosleep(void) { return knanosleep(1000); }

struct syscall_entry {
    void* handler;
    u8 nargs;
    char const* name;
};

typedef int (*syscall_fn_0)(void);
typedef int (*syscall_fn_1)(long);
typedef int (*syscall_fn_2)(long, long);
typedef int (*syscall_fn_3)(long, long, long);
typedef int (*syscall_fn_5)(long, long, long, long, long);

static const struct syscall_entry syscall_table[NR_SYSCALLS] = {
    SYSCALL_ENTRY_1(SYS_EXIT, exit),
    SYSCALL_ENTRY_3(SYS_READ, read),
    SYSCALL_ENTRY_3(SYS_WRITE, write),
    SYSCALL_ENTRY_3(SYS_OPEN, open),
    SYSCALL_ENTRY_1(SYS_CLOSE, close),
    SYSCALL_ENTRY_3(SYS_WAITPID, waitpid),
    SYSCALL_ENTRY_1(SYS_UNLINK, unlink),
    SYSCALL_ENTRY_1(SYS_CHDIR, chdir),
    SYSCALL_ENTRY_1(SYS_TIME, time),
    SYSCALL_ENTRY_3(SYS_MKNOD, mknod),
    SYSCALL_ENTRY_2(SYS_STAT, stat),
    SYSCALL_ENTRY_3(SYS_LSEEK, lseek),
    SYSCALL_ENTRY_0(SYS_GETPID, getpid),
    SYSCALL_ENTRY_5(SYS_MOUNT, mount),
    SYSCALL_ENTRY_1(SYS_SETUID, setuid),
    SYSCALL_ENTRY_1(SYS_GETUID, getuid),
    SYSCALL_ENTRY_2(SYS_FSTAT, fstat),
    SYSCALL_ENTRY_2(SYS_KILL, kill),
    SYSCALL_ENTRY_2(SYS_MKDIR, mkdir),
    SYSCALL_ENTRY_1(SYS_RMDIR, rmdir),
    SYSCALL_ENTRY_1(SYS_SETGID, setgid),
    SYSCALL_ENTRY_0(SYS_GETGID, getgid),
    SYSCALL_ENTRY_0(SYS_GETEUID, geteuid),
    SYSCALL_ENTRY_0(SYS_GETEGID, getegid),
    SYSCALL_ENTRY_2(SYS_UMOUNT, umount),
    SYSCALL_ENTRY_2(SYS_SETREUID, setreuid),
    SYSCALL_ENTRY_2(SYS_SETREGID, setregid),
    SYSCALL_ENTRY_2(SYS_SETGROUPS, setgroups),
    SYSCALL_ENTRY_2(SYS_GETGROUPS, getgroups),
    SYSCALL_ENTRY_3(SYS_READDIR, readdir),
    SYSCALL_ENTRY_2(SYS_TRUNCATE, truncate),
    SYSCALL_ENTRY_2(SYS_FTRUNCATE, ftruncate),
    SYSCALL_ENTRY_2(SYS_SOCKETCALL, socketcall),
    SYSCALL_ENTRY_3(SYS_INIT_MODULE, init_module),
    SYSCALL_ENTRY_2(SYS_DELETE_MODULE, delete_module),
    SYSCALL_ENTRY_1(SYS_FCHDIR, fchdir),
    SYSCALL_ENTRY_0(SYS_NANOSLEEP, nanosleep),
    SYSCALL_ENTRY_3(SYS_SETRESUID, setresuid),
    SYSCALL_ENTRY_3(SYS_SETRESGID, setresgid),
    SYSCALL_ENTRY_2(SYS_GETCWD, getcwd),
};

__attribute__((target("general-regs-only"))) void
syscall_dispatcher_c(trapframe_t* reg)
{
    sti();

    u32 syscall_num = reg->eax;
    if (syscall_num == 0 || syscall_num >= NR_SYSCALLS) {
        reg->eax = -ENOSYS;
        check_resched();
        return;
    }

    int ret;
    const struct syscall_entry* entry = &syscall_table[syscall_num];

    switch (syscall_num) {
    case SYS_EXECVE:
        if (reg->cs != USER_CS) {
            ret = -EINVAL;
            break;
        }

        ret = sys_execve(
            (char*)reg->ebx, (char const* const*)reg->ecx,
            (char const* const*)reg->edx, reg
        );
        break;

    case SYS_FORK:
        ret = sys_fork(reg);
        break;

    case SYS_SIGNAL:
    case SYS_GETRESUID:
    case SYS_GETRESGID:
        ret = -ENOSYS;
        break;

    default: {
        switch (entry->nargs) {
        case 0:
            ret = ((syscall_fn_0)entry->handler)();
            break;
        case 1:
            ret = ((syscall_fn_1)entry->handler)(reg->ebx);
            break;
        case 2:
            ret = ((syscall_fn_2)entry->handler)(reg->ebx, reg->ecx);
            break;
        case 3:
            ret = ((syscall_fn_3)entry->handler)(reg->ebx, reg->ecx, reg->edx);
            break;
        case 5:
            ret = ((syscall_fn_5
            )entry->handler)(reg->ebx, reg->ecx, reg->edx, reg->esi, reg->edi);
            break;
        default:
            ret = -ENOSYS;
            break;
        }
    }
    }

    reg->eax = ret;
    check_resched();
}
