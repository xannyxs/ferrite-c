#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define MAX_PROCS 64

typedef int32_t pid_t;

typedef struct {
  uint32_t edi;
  uint32_t esi;
  uint32_t ebx;
  uint32_t ebp;
  uint32_t eip;
} context_t;

typedef enum {
  UNUSED,
  EMBRYO,
  SLEEPING,
  RUNNABLE,
  RUNNING,
  ZOMBIE
} proc_state_e;

typedef struct trapframe {
  // registers as pushed by pusha
  uint32_t edi, esi, ebp;
  uint32_t esp_dummy; // useless & ignored
  uint32_t ebx, edx, ecx, eax;

  // rest of trap frame
  uint16_t gs, padding1;
  uint16_t fs, padding2;
  uint16_t es, padding3;
  uint16_t ds, padding4;
  uint32_t trapno;

  // below here defined by x86 hardware
  uint32_t err;
  uint32_t eip;
  uint16_t cs, padding5;
  uint32_t eflags;

  // below here only when crossing rings, such as from user to kernel
  uint32_t esp;
  uint16_t ss, padding6;
} trapframe_t;

typedef struct proc {
  pid_t pid;             // Process ID
  proc_state_e state;    // Process state
  char *kstack;          // Bottom of kernel stack
  trapframe_t *trap;     // Trap frame for current syscall
  struct proc *parent;   // Parent process
  context_t *context;    // Switch here to run process
  uint32_t memory_limit; // Size of process memory (bytes)

#ifndef DEBUG
  char name[16]; // Process name (debugging)
#endif
} proc_t;

pid_t sys_fork(void);

void proc_init(void);

proc_t *proc_alloc(void);

proc_t *myproc(void);

#endif /* PROCESS_H */
