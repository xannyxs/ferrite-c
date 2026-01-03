#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "arch/x86/idt/idt.h"

#define USER_CS 0x23

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
    SYS_EXECVE = 11,
    SYS_CHDIR = 12,
    SYS_TIME = 13,
    SYS_STAT = 18,
    SYS_LSEEK = 19,
    SYS_GETPID = 20,

    SYS_MOUNT = 22,

    SYS_SETUID = 23,
    SYS_GETUID = 24,

    SYS_KILL = 37,
    SYS_MKDIR = 39,
    SYS_RMDIR = 40,

    SYS_SETGID = 46,
    SYS_GETGID = 47,

    SYS_SIGNAL = 48,

    SYS_GETEUID = 49,
    SYS_GETEGID = 50,

    SYS_UMOUNT = 52,

    SYS_SETREUID = 70,
    SYS_SETREGID = 71,
    SYS_GETGROUPS = 80,
    SYS_SETGROUPS = 81,

    SYS_READDIR = 89,
    SYS_TRUNCATE = 92,
    SYS_FTRUNCATE = 93,
    SYS_SOCKETCALL = 102,

    SYS_INIT_MODULE = 128,
    SYS_DELETE_MODULE = 129,

    SYS_FCHDIR = 133,
    SYS_NANOSLEEP = 162,

    SYS_SETRESUID = 164,
    SYS_GETRESUID = 165,
    SYS_SETRESGID = 170,
    SYS_GETRESGID = 171,

    SYS_GETCWD = 183,
};

void syscall_dispatcher_c(registers_t*);

int sys_socketcall(int call, unsigned long* args);

/* sys.c */

/* Mount */

SYSCALL_ATTR int sys_mount(char*, char*, char*, unsigned long, void*);

SYSCALL_ATTR int sys_umount(char const* name, int flags);

/* UID */

SYSCALL_ATTR uid_t sys_getuid(void);

SYSCALL_ATTR uid_t sys_geteuid(void);

SYSCALL_ATTR s32 sys_setuid(uid_t uid);

SYSCALL_ATTR s32 sys_seteuid(uid_t uid);

SYSCALL_ATTR s32 sys_setreuid(uid_t ruid, uid_t euid);

SYSCALL_ATTR s32 sys_setresuid(uid_t ruid, uid_t euid, uid_t suid);

/* GID */

SYSCALL_ATTR uid_t sys_getgid(void);

SYSCALL_ATTR uid_t sys_getegid(void);

SYSCALL_ATTR uid_t sys_getsgid(void);

SYSCALL_ATTR s32 sys_setgid(gid_t gid);

SYSCALL_ATTR s32 sys_setegid(uid_t gid);

SYSCALL_ATTR s32 sys_setregid(gid_t rgid, gid_t egid);

SYSCALL_ATTR s32 sys_setresgid(gid_t rgid, gid_t egid, gid_t sgid);

/* GROUPS */

SYSCALL_ATTR gid_t sys_getgroups(gid_t* grouplist, int len);

SYSCALL_ATTR gid_t sys_setgroups(gid_t* grouplist, int len);

/* modules.c */

/* modules */

int sys_delete_module(char const* name_user, unsigned int flags);

int sys_init_module(void* mod, unsigned long len, char* const args);

#endif /* SYSCALLS_H */
