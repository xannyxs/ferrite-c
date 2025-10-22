#include "arch/x86/gdt/gdt.h"
#include "arch/x86/memlayout.h"
#include "debug/debug.h"
#include "drivers/printk.h"
#include "lib/stdlib.h"
#include "lib/string.h"
#include "memory/consts.h"
#include "memory/page.h"
#include "memory/vmm.h"
#include "sys/process/process.h"
#include "types.h"

#ifdef __TEST
#    include "tests/tests.h"
#endif

#include <stdbool.h>

proc_t* initial_proc;

extern proc_t ptables[NUM_PROC];
extern u32 pid_counter;
extern u32 page_directory[1024];
extern void jump_to_usermode(void* entry, void* user_stack);

__attribute__((naked)) void user_init(void)
{
    __asm__ volatile("xchg %bx, %bx;"

                     "mov $20, %eax;" // SYS_GETPID in EAX
                     "int $0x80;"

                     "mov %eax, %ebx;" // Save the PID in EBX for the next call
                     "mov $1, %eax;"   // SYS_EXIT in EAX
                     "int $0x80;"

                     ".hang:"
                     "jmp .hang");
}

/* Public */

inline proc_t* initproc(void) { return initial_proc; }

void prepare_for_jmp(void)
{
    void* page_kaddr = get_free_page();
    if (!page_kaddr) {
        abort("Out of physical memory for user code");
    }

    paddr_t paddr = V2P_WO((u32)page_kaddr);
    void* user_code_vaddr = (void*)0x10000000;
    void* user_stack_vaddr = (void*)0xBFFFF000;

    u32 user_init_addr = (u32)user_init;
    memcpy(page_kaddr, (void*)user_init_addr, PAGE_SIZE);

    if (vmm_map_page((void*)paddr, user_code_vaddr, PTE_P | PTE_W | PTE_U)
        < 0) {
        abort("Failed to map user code page");
    }

    if (vmm_map_page(NULL, user_stack_vaddr, PTE_P | PTE_W | PTE_U) < 0) {
        abort("Failed to map user stack page");
    }

    jump_to_usermode(user_code_vaddr, (void*)0xBFFFFFFC);
}

void init_process(void)
{
    proc_t const* current = myproc();
    if (current->pid != 1) {
        abort("Init process should be PID 1!");
    }

    printk("Initial process started...!\n");

    pid_t pid = do_exec("user space", prepare_for_jmp);
    if (pid < 0) {
        abort("Init: could not create a new process");
    }

    pid = do_exec("shell", shell_process);
    if (pid < 0) {
        abort("Init: could not create a new process");
    }

#ifdef __TEST
    pid = do_exec("test", main_tests);
    if (pid < 0) {
        abort("Init: could not create a new process");
    }
#endif

    printk("Init: Created child PID %d with PID %d\n", pid, current->pid);

    while (true) {
        for (s32 i = 0; i < NUM_PROC; i++) {
            proc_t* p = &ptables[i];
            if (p->state == ZOMBIE && p->parent == current) {
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
