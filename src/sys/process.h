#ifndef PROCESS_H
#define PROCESS_H

#define NUM_PROC 64

#include <stdint.h>

typedef int32_t pid_t;
typedef enum { UNUSED, EMBRYO, SLEEPING, READY, RUNNING, ZOMBIE } procstate_e;
typedef struct {
  uint32_t eax, ebx, ecx, edx;
  uint32_t esi, edi;
  uint32_t ebp, esp;
  uint32_t eip;
  uint32_t eflags;
  uint32_t cr3;
} context_t;

typedef struct process {
  pid_t pid;
  procstate_e state;
  context_t *context;

  void *pgdir;
  uint32_t sz;

  char *kstack;

  struct process *parent;

#ifndef __DEBUG
  char name[16];
#endif
} proc_t;

void schedule(void);

void create_first_process(void);

void init_ptables(void);

void yield(void);

pid_t fork(void);

#endif /* PROCESS_H */
