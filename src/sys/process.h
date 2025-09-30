#ifndef PROCESS_H
#define PROCESS_H

#include "arch/x86/pit.h"

#include <stdint.h>

#define NUM_PROC 64
#define TIME_QUANTUM (100 * HZ / 1000)

typedef int32_t pid_t;
typedef enum { UNUSED, EMBRYO, SLEEPING, READY, RUNNING, ZOMBIE } procstate_e;
typedef struct {
  uint32_t edi, esi, ebx, ebp, eip;
} context_t;

typedef struct process {
  pid_t pid;
  procstate_e state;

  context_t *context;

  void *pgdir;
  uint32_t sz;

  uint32_t pending_signals;

  char *kstack;
  struct process *parent;
  void *channel;
  int32_t status;
  char name[16];
} proc_t;

extern void swtch(context_t **old, context_t *new);

void schedule(void);

void create_initial_process(void);

void init_ptables(void);

void yield(void);

/**
 * Creates a child process that is an exact copy of the current process.
 * Both parent and child resume execution from the point where do_fork() was
 * called.
 *
 * @param name  Process name for the child process
 * @return      Child PID in parent process, 0 in child process, -1 on error
 */
pid_t do_fork(const char *name);

/**
 * Creates a new process that starts executing the specified function.
 * The new process begins execution at function f, not at the call site.
 *
 * @param name  Process name for the new process
 * @param f     Function pointer where the new process should start executing
 * @return      New process PID in calling process, -1 on error
 */
pid_t do_exec(const char *name, void (*f)(void));

void do_exit(int32_t status);

pid_t do_wait(int32_t *status);

proc_t *myproc(void);

void process_list(void);

proc_t *find_process(pid_t pid);

void check_resched(void);

void *setup_kvm(void);

/* Main process */

void shell_process(void);

void init_process(void);

#endif /* PROCESS_H */
