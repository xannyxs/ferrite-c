#include "sys/process.h"
#include "arch/x86/io.h"
#include "arch/x86/memlayout.h"
#include "debug/debug.h"
#include "drivers/printk.h"
#include "lib/stdlib.h"
#include "lib/string.h"
#include "memory/consts.h"
#include "memory/page.h"
#include "memory/vmm.h"
#include "sys/signal/signal.h"
#include "sys/timer.h"
#include "types.h"

#include <stdbool.h>

extern u32 page_directory[1024];

proc_t ptables[NUM_PROC];
proc_t* current_proc = NULL;
context_t* scheduler_context;
s32 pid_counter = 1;

s32 ticks_remaining;
bool volatile need_resched = false;

/* Private */

static inline void inherit_credentials(proc_t* child, proc_t* parent)
{
    if (parent) {
        child->uid = parent->uid;
        child->euid = parent->euid;
        return;
    }

    child->uid = child->euid = ROOT_UID;
}

proc_t* __alloc_proc(void)
{
    for (s32 i = 0; i < NUM_PROC; i += 1) {
        if (ptables[i].state == UNUSED) {
            proc_t* p = &ptables[i];

            inherit_credentials(p, current_proc);
            p->state = EMBRYO;
            p->pid = pid_counter;
            pid_counter += 1;

            p->kstack = get_free_page();
            if (!p->kstack) {
                p->state = UNUSED;
                return NULL;
            }

            p->pgdir = setup_kvm();
            if (!p->pgdir) {
                p->state = UNUSED;
                free_page(p->kstack);
                return NULL;
            }

            p->parent = current_proc;

            return p;
        }
    }

    return NULL;
}

/* Public */

inline proc_t* myproc(void) { return current_proc; }

// Real UID: actual user who started the process (for accounting/auditing)
inline uid_t getuid(void)
{
    return current_proc->uid;
}

// Effective UID: determines permissions for system access (files, syscalls, etc.)
inline uid_t geteuid(void)
{
    return current_proc->euid;
}

inline proc_t* find_process(pid_t pid)
{
    for (s32 i = 0; i < NUM_PROC; i += 1) {
        if (ptables[i].pid == pid) {
            return &ptables[i];
        }
    }

    return NULL;
}

void init_ptables(void)
{
    for (s32 i = 0; i < NUM_PROC; i += 1) {
        ptables[i].state = UNUSED;
    }
}

inline void check_resched(void)
{
    proc_t* p = myproc();
    if (!p || need_resched == false) {
        return;
    }

    printk("Preempting PID %d\n", current_proc->pid);
    need_resched = false;
    yield();
}

__attribute__((naked)) static void fork_ret(void)
{
    __asm__ volatile("movl $0, %%eax\n\t"
                     "popl %%ecx\n\t"
                     "jmp *%%ecx" ::
                         : "eax", "ecx");
}

void wakeup(void* channel)
{
    for (proc_t* p = &ptables[0]; p < &ptables[NUM_PROC]; p += 1) {
        if (p->channel == channel && p->state == SLEEPING) {
            p->state = READY;
        }
    }
}

void do_exit(s32 status)
{
    proc_t* p = myproc();
    proc_t* init = initproc();

    for (s32 i = 0; i < NUM_PROC; i++) {
        if (ptables[i].parent != p) {
            continue;
        }

        ptables[i].parent = init;
        if (ptables[i].state == ZOMBIE) {
            wakeup(init);
        }
    }

    cli();

    p->status = status;
    wakeup(p->parent);
    p->state = ZOMBIE;

    swtch(&p->context, scheduler_context);
    __builtin_unreachable();
}

pid_t do_wait(s32* status)
{
    while (true) {
        bool have_kids = false;

        for (s32 i = 0; i < NUM_PROC; i += 1) {
            proc_t* p = &ptables[i];

            if (p->parent != myproc()) {
                continue;
            }

            have_kids = true;
            if (p->state == ZOMBIE) {
                pid_t pid = p->pid;
                if (status) {
                    *status = p->status;
                }

                free_page(p->kstack);
                p->kstack = NULL;

                vmm_free_pagedir(p->pgdir);
                p->pgdir = NULL;

                p->state = UNUSED;
                p->pid = 0;
                p->parent = NULL;

                return pid;
            }
        }

        if (!have_kids) {
            return -1;
        }

        sleeppid(myproc());
    }
}

void* setup_kvm(void)
{
    u32* pgdir = (u32*)get_free_page();
    if (!pgdir) {
        return NULL;
    }

    for (int i = 0; i < 1024; i++) {
        pgdir[i] = page_directory[i];
    }

    pgdir[1023] = V2P_WO((u32)pgdir) | PTE_P | PTE_W | PTE_U;

    return pgdir;
}

pid_t do_exec(char const* name, void (*f)(void))
{
    proc_t* p = __alloc_proc();
    if (!p) {
        return -1;
    }

    u32* ctx = (u32*)(p->kstack + PAGE_SIZE);
    *(--ctx) = (u32)f; // EIP
    *(--ctx) = 0;      // EBP
    *(--ctx) = 0;      // EBX
    *(--ctx) = 0;      // ESI
    *(--ctx) = 0;      // EDI
    p->context = (context_t*)ctx;

    strlcpy(p->name, name, sizeof(p->name));
    p->state = READY;

    return p->pid;
}

pid_t do_fork(char const* name)
{
    proc_t* p = __alloc_proc();
    if (!p) {
        return -1;
    }

    u32 caller_return;
    __asm__ volatile("movl 4(%%ebp), %0" : "=r"(caller_return));
    u32* ctx = (u32*)(p->kstack + PAGE_SIZE);
    *(--ctx) = caller_return; // Return address for fork_ret (on stack)
    *(--ctx) = (u32)fork_ret; // EIP
    *(--ctx) = 0;             // EBP
    *(--ctx) = 0;             // EBX
    *(--ctx) = 0;             // ESI
    *(--ctx) = 0;             // EDI
    p->context = (context_t*)ctx;

    strlcpy(p->name, name, sizeof(p->name));
    p->state = READY;

    return p->pid;
}

inline void yield(void)
{
    proc_t* p = myproc();
    if (!p) {
        return;
    }

    need_resched = false;
    p->state = READY;
    swtch(&p->context, scheduler_context);
}

void schedule(void)
{
    static char* scheduler_stack = NULL;
    if (!scheduler_stack) {
        scheduler_stack = get_free_page();
        if (!scheduler_stack) {
            abort("Cannot allocate scheduler stack");
        }
        scheduler_context = (context_t*)(scheduler_stack + PAGE_SIZE);
    }

    // FIFO - Round Robin
    while (true) {
        for (s32 i = 0; i < NUM_PROC; i += 1) {
            if (ptables[i].state != READY) {
                continue;
            }
            proc_t* p = &ptables[i];
            current_proc = p;

            handle_signal();
            if (p->state != READY && p->state != RUNNING) {
                current_proc = NULL;
                continue;
            }

            p->state = RUNNING;
            ticks_remaining = TIME_QUANTUM;

            lcr3(V2P_WO((u32)p->pgdir));
            swtch(&scheduler_context, p->context);
            lcr3(V2P_WO((u32)page_directory));

            if (current_proc && current_proc->state == RUNNING) {
                current_proc->state = READY;
            }

            current_proc = NULL;
        }

        sti();
        __asm__ volatile("hlt");
    }
}
