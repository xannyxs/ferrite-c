#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

typedef int32_t pid_t;

typedef enum {
  UNUSED,
  EMBRYO,
  SLEEPING,
  RUNNABLE,
  RUNNING,
  ZOMBIE
} procstate_e;

typedef struct {
  pid_t pid;               // Process ID
  uint32_t sz;             // Size of process memory (bytes)
  char *kstack;            // Bottom of kernel stack for this process
  procstate_e state;       // Process state
  struct pcb_t *parent;    // Parent process
  struct trapframe *tf;    // Trap frame for current syscall
  struct context *context; // swtch() here to run process
  void *chan;              // If non-zero, sleeping on chan
  int32_t killed;          // If non-zero, have been killed
  struct inode *cwd;       // Current directory

  // pde_t *pgdir;            // Page table

#if defined(__debug)
  char name[16]; // Process name (debugging)
#endif
} pcb_t;

#endif /* PROCESS_H */
