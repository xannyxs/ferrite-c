#ifndef CPU_H
#define CPU_H

#include "arch/x86/gdt/gdt.h"
#include "sys/process.h"

#include <stdbool.h>
#include <stdint.h>

// TODO: In xv6 there struct looks like this:
// struct cpu {
//   uchar apicid;                // Local APIC ID
//   struct context *scheduler;   // swtch() here to enter scheduler
//   struct taskstate ts;         // Used by x86 to find stack for interrupt
//   struct segdesc gdt[NSEGS];   // x86 global descriptor table
//   volatile uint started;       // Has the CPU started?
//   int ncli;                    // Depth of pushcli nesting.
//   int intena;                  // Were interrupts enabled before pushcli?
//   struct proc *proc;           // The process running on this cpu or null
// };
//
// We might need to look into the other variables in the future
// for multithreading

typedef struct cpu {
  context_t scheduler_context;
  context_t *scheduler; // swtch() here to enter scheduler
  tss_entry_t ts;       // Used by x86 to find stack for interrupt
  proc_t *proc;         // The process running on this cpu or null
  int32_t ncli;         // Depth of cli nesting.
  int32_t intena;       // Were interrupts enabled before cli?
} cpu_t;

void cpu_init(void);

void scheduler(void);

void yield(void);

#endif /* CPU_H */
