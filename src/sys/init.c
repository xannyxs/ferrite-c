#include "drivers/printk.h"
#include "lib/stdlib.h"
#include "lib/string.h"
#include "memory/consts.h"
#include "memory/page.h"
#include "memory/vmm.h"
#include "sys/process.h"

#ifdef __TEST
#    include "tests/tests.h"
#endif

#include <stdbool.h>
#include <stdint.h>

extern proc_t ptables[NUM_PROC];
extern uint32_t pid_counter;
extern uint32_t page_directory[1024];

proc_t* initial_proc;

void init_process(void)
{
    proc_t const* current_proc = myproc();
    if (current_proc->pid != 1) {
        abort("Init process should be PID 1!");
    }

    printk("Initial process started...!\n");

    pid_t pid = do_exec("shell", shell_process);
    if (pid < 0) {
        abort("Init: could not create a new process");
    } else if (pid == 0) {
        shell_process();
        __builtin_unreachable();
    }

#ifdef __TEST
    pid = do_exec("test", main_tests);
    if (pid < 0) {
        abort("Init: could not create a new process");
    }
#endif

    printk("Init: Created child PID %d with PID %d\n", pid, current_proc->pid);

    while (true) {
        for (int32_t i = 0; i < NUM_PROC; i++) {
            proc_t* p = &ptables[i];
            if (p->state == ZOMBIE && p->parent == current_proc) {
                p->state = UNUSED;
                free_page(p->kstack);
                vmm_free_pagedir(p->pgdir);

                p->pgdir = NULL;
                p->kstack = NULL;

                printk("Init: reaped zombie PID %d\n", p->pid);
            }
        }

        yield();
    }
}

proc_t* initproc(void) { return initial_proc; }

void create_initial_process(void)
{
    proc_t* init = __alloc_proc();
    if (!init) {
        abort("create_initial_process: something went wrong on initiating the "
              "process");
    }

    uint32_t* sp = (uint32_t*)((char*)init->kstack + PAGE_SIZE);
    *--sp = (uint32_t)init_process; // EIP - function to execute
    *--sp = 0;                      // EBP
    *--sp = 0;                      // EBX
    *--sp = 0;                      // ESI
    *--sp = 0;                      // EDI
    init->context = (context_t*)sp;

    strlcpy(init->name, "init", sizeof(init->name));
    init->state = READY;

    initial_proc = init;
}
