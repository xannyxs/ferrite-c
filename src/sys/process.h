#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

typedef int32_t pid_t;

typedef struct {
  int32_t eip, esp, ebx, ecx, edx, esi, edi, ebp;
} context_t;

typedef enum {
  UNUSED,
  EMBRYO,
  SLEEPING,
  RUNNABLE,
  RUNNING,
  ZOMBIE
} proc_state_e;

typedef struct proc {
  pid_t pid;           // Process ID
  char *mem;           // Start of process memory
  uint32_t sz;         // Size of process memory
  char *kstack;        // Bottom of kernel stack
  proc_state_e state;  // Process state
  struct proc *parent; // Parent process
  context_t context;   // Switch here to run process
} proc_t;

#endif /* PROCESS_H */
