#include "arch/x86/gdt/gdt.h"
#include "arch/x86/memlayout.h"
#include "drivers/printk.h"
#include "fs/vfs.h"
#include "idt/syscalls.h"
#include "io.h"
#include "lib/stdlib.h"
#include "memory/consts.h"
#include "memory/page.h"
#include "memory/vmm.h"
#include "sys/file/fcntl.h"
#include "sys/process/process.h"

#include <ferrite/string.h>
#include <ferrite/types.h>

#ifdef __TEST
#    include "tests/tests.h"
#endif

proc_t* initial_proc;

extern vfs_inode_t* root_inode;
extern proc_t ptables[NUM_PROC];
extern u32 pid_counter;
extern u32 page_directory[1024];
extern void jump_to_usermode(void* entry, void* user_stack);

__attribute__((naked)) void user_init(void)
{
    __asm__ volatile(
        // Get current position (PIC trick for 32-bit)
        "call 1f\n"
        "1: pop %%ebx\n" // EBX = current address

        // Set up argv on stack
        "sub $16, %%esp\n"
        "movl $0, 12(%%esp)\n"
        "lea (.sh_str-1b)(%%ebx), %%eax\n"
        "movl %%eax, 8(%%esp)\n"
        "lea 8(%%esp), %%ecx\n"

        // Set up envp
        "movl $0, 4(%%esp)\n"
        "lea 4(%%esp), %%edx\n"

        // Call execve
        "movl $11, %%eax\n" // SYS_EXECVE
        "lea (.sh_path-1b)(%%ebx), %%ebx\n"
        "int $0x80\n"

        // If execve fails, exit
        "mov %%eax, %%ebx\n"
        "mov $1, %%eax\n" // SYS_EXIT
        "int $0x80\n"

        ".sh_path: .asciz \"/bin/sh\"\n"
        ".sh_str:  .asciz \"shell\"\n"
        :
        :
        : "memory"
    );
}

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

/* Public */

inline proc_t* initproc(void) { return initial_proc; }

void init_process(void)
{
    proc_t const* current = myproc();
    if (current->pid != 1) {
        abort("Init process should be PID 1!");
    }

    sti();

    devfs_init();
    printk("Initial process started...!\n");

    int stdin_fd = sys_open("/dev/console", O_RDONLY, 0);
    int stdout_fd = sys_open("/dev/console", O_WRONLY, 0);
    int stderr_fd = sys_open("/dev/console", O_WRONLY, 0);

    if (stdin_fd != 0 || stdout_fd != 1 || stderr_fd != 2) {
        abort("Failed to open stdio!");
    }

    pid_t pid = do_exec("execve test", prepare_for_jmp);
    if (pid < 0) {
        abort("Init: could not create execve test process");
    }

#ifdef __TEST
    pid = do_exec("test", main_tests);
    if (pid < 0) {
        abort("Init: could not create a new process");
    }
#endif

    while (1) {
        for (s32 i = 0; i < NUM_PROC; i++) {
            proc_t* p = &ptables[i];
            if (p->state == ZOMBIE && p->parent == current) {
                p->state = UNUSED;
                free_page(p->kstack);
                vmm_free_pagedir(p->pgdir);

                p->pgdir = NULL;
                p->kstack = NULL;

                printk(
                    "Init: reaped zombie PID %d with error code: %d\n", p->pid,
                    p->status
                );
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
