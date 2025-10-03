#include "arch/x86/gdt/gdt.h"
#include "debug/debug.h"
#include "drivers/printk.h"
#include "lib/stdlib.h"
#include "lib/string.h"
#include "memory/consts.h"
#include "memory/page.h"
#include "memory/vmm.h"
#include "sys/process.h"
#include "types.h"

#ifdef __TEST
#    include "tests/tests.h"
#endif

#include <stdbool.h>

extern proc_t ptables[NUM_PROC];
extern u32 pid_counter;
extern u32 page_directory[1024];

extern void jump_to_usermode(void* entry, void* user_stack);

proc_t* initial_proc;

void user_init(void)
{
    s32 my_pid;
    __asm__ volatile(
        "int $0x80"
        : "=a"(my_pid)
        : "a"(20) // SYS_GETPID in EAX
    );

    __asm__ volatile(
        "int $0x80"
        :
        : "a"(1), "b"(my_pid) // SYS_EXIT=0
    );

    while (1) {
        __asm__ volatile(
            "int $0x80"
            : "=a"(my_pid)
            : "a"(20) // SYS_GETPID in EAX
        );
    }
}

/* Public */

inline proc_t* initproc(void) { return initial_proc; }

#include "arch/x86/memlayout.h"

void init_process(void)
{
    proc_t const* current_proc = myproc();
    if (current_proc->pid != 1) {
        abort("Init process should be PID 1!");
    }

    printk("Initial process started...!\n");

    pid_t pid = do_fork("user space");
    if (pid < 0) {
        abort("Init: could not create a new process");
    } else if (pid == 0) {
        void* user_code_addr = (void*)0x400000;
        memcpy(user_code_addr, (void*)(u32)user_init, 4096);

        vmm_map_page(NULL, (void*)0xBFFFF000, PTE_P | PTE_W | PTE_U);

        printk("Second\n");
        process_list();
        jump_to_usermode((void*)0x400000, (void*)0xC0000000);
    }

    pid = do_exec("shell", shell_process);
    if (pid < 0) {
        abort("Init: could not create a new process");
    }
    printk("First\n");
    process_list();

#ifdef __TEST
    pid = do_exec("test", main_tests);
    if (pid < 0) {
        abort("Init: could not create a new process");
    }
#endif

    printk("Init: Created child PID %d with PID %d\n", pid, current_proc->pid);

    while (true) {
        for (s32 i = 0; i < NUM_PROC; i++) {
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

void create_initial_process(void)
{
    proc_t* init = __alloc_proc();
    if (!init) {
        abort("create_initial_process: something went wrong on initiating the "
              "process");
    }

    u32* sp = (u32*)(init->kstack + PAGE_SIZE);
    *--sp = (u32)init_process; // EIP - function to execute
    *--sp = 0;                 // EBP
    *--sp = 0;                 // EBX
    *--sp = 0;                 // ESI
    *--sp = 0;                 // EDI
    init->context = (context_t*)sp;

    strlcpy(init->name, "init", sizeof(init->name));
    init->state = READY;

    initial_proc = init;

    u32 kernel_stack_top = (u32)init->kstack + PAGE_SIZE;
    tss_set_stack(kernel_stack_top);
}
