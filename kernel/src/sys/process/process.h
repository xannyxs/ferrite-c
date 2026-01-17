#ifndef PROCESS_H
#define PROCESS_H

#include "arch/x86/pit.h"
#include "ferrite/limits.h"
#include "fs/vfs.h"
#include "idt/idt.h"
#include "sys/file/file.h"

#include <ferrite/types.h>

#define MAX_OPEN_FILES 64
#define NUM_PROC 32
#define TIME_QUANTUM (100 * HZ / 1000)
#define ROOT_UID 0

typedef s32 pid_t;

typedef enum { UNUSED, EMBRYO, SLEEPING, READY, RUNNING, ZOMBIE } procstate_e;

typedef struct {
    u32 edi, esi, ebx, ebp, eip;
} context_t;

typedef struct process {
    pid_t pid;

    uid_t uid;
    uid_t euid;
    uid_t suid;

    gid_t gid;
    gid_t egid;
    gid_t sgid;
    int groups[NGROUPS];

    procstate_e state;

    context_t* context;

    void* pgdir;
    char* kstack;

    void* channel;
    u32 pending_signals;
    s32 status;

    vfs_inode_t* root;
    vfs_inode_t* pwd;

    file_t* open_files[MAX_OPEN_FILES];

    struct process* parent;
    char name[16];
} proc_t;

extern void swtch(context_t** old, context_t* new);

void schedule(void);

void create_initial_process(void);

void ptables_init(void);

void yield(void);

/**
 * Creates a child process that is an exact copy of the current process.
 * Both parent and child resume execution from the point where do_fork() was
 * called.
 *
 * @param name  Process name for the child process
 * @return      Child PID in parent process, 0 in child process, -1 on error
 */
pid_t do_fork(trapframe_t*, char const*);

/**
 * Creates a new process that starts executing the specified function.
 * The new process begins execution at function f, not at the call site.
 *
 * @param name  Process name for the new process
 * @param f     Function pointer where the new process should start executing
 * @return      New process PID in calling process, -1 on error
 */
pid_t do_exec(char const* name, void (*f)(void));

void do_exit(s32 status);

void wakeup(void* channel);

pid_t do_wait(s32* status);

void process_list(void);

proc_t* find_process(pid_t pid);

void check_resched(void);

void* setup_kvm(void);

proc_t* __alloc_proc(void);

proc_t* myproc(void);

proc_t* initproc(void);

/* Main process */

void shell_process(void);

void init_process(void);

#endif /* PROCESS_H */
