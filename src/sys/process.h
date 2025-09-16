#ifndef PROCESS_H
#define PROCESS_H

#define NUM_PROC 64
#define TIME_QUANTUM 5

#include <stdint.h>

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

  char *kstack;
  struct process *parent;
  char name[16];
} proc_t;

extern void swtch(context_t **old, context_t *new);

void schedule(void);

void create_initial_process(void);

void init_ptables(void);

void yield(void);

pid_t fork(char *name, void (*f)(void));

proc_t *myproc(void);

// pid_t fork(void);

void process_list(void);

#endif /* PROCESS_H */
