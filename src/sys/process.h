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

typedef struct {
  // registers as pushed by pusha
  uint32_t edi;
  uint32_t esi;
  uint32_t ebp;
  uint32_t oesp; // useless & ignored
  uint32_t ebx;
  uint32_t edx;
  uint32_t ecx;
  uint32_t eax;

  // rest of trap frame
  uint16_t gs;
  uint16_t padding1;
  uint16_t fs;
  uint16_t padding2;
  uint16_t es;
  uint16_t padding3;
  uint16_t ds;
  uint16_t padding4;
  uint32_t trapno;

  // below here defined by x86 hardware
  uint32_t err;
  uint32_t eip;
  uint16_t cs;
  uint16_t padding5;
  uint32_t eflags;

  // below here only when crossing rings, such as from user to kernel
  uint32_t esp;
  uint16_t ss;
  uint16_t padding6;
} trapframe_t;

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

pid_t do_fork(char *name);

proc_t *myproc(void);

void process_list(void);

void check_resched(void);

/* Main process */

void shell_process(void);

void init_process(void);

#endif /* PROCESS_H */
