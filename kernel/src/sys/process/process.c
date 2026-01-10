#include "sys/process/process.h"
#include "arch/x86/gdt/gdt.h"
#include "arch/x86/io.h"
#include "arch/x86/memlayout.h"
#include "debug/debug.h"
#include "drivers/printk.h"
#include "fs/vfs.h"
#include "lib/stdlib.h"
#include "memory/consts.h"
#include "memory/page.h"
#include "memory/vmm.h"
#include "sys/file/file.h"
#include "sys/signal/signal.h"
#include "sys/timer/timer.h"

#include <ferrite/string.h>
#include <ferrite/types.h>
#include <stdbool.h>

extern vfs_inode_t* root_inode;
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
        child->suid = parent->suid;

        child->gid = parent->gid;
        child->egid = parent->egid;
        child->sgid = parent->sgid;

        return;
    }

    child->uid = child->euid = child->suid = ROOT_UID;
    child->gid = child->egid = child->sgid = ROOT_UID;
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
            p->root = current_proc ? current_proc->root : root_inode;
            p->root->i_count += 1;

            p->pwd = current_proc ? current_proc->pwd : root_inode;
            p->pwd->i_count += 1;

            for (int fd = 0; fd < MAX_OPEN_FILES; fd += 1) {
                p->open_files[fd] = NULL;

                if (current_proc && current_proc->open_files[fd]) {
                    p->open_files[fd] = current_proc->open_files[fd];
                    p->open_files[fd]->f_count += 1;
                }
            }

            return p;
        }
    }

    return NULL;
}

/* Public */

inline proc_t* myproc(void) { return current_proc; }

inline proc_t* find_process(pid_t pid)
{
    for (s32 i = 0; i < NUM_PROC; i += 1) {
        if (ptables[i].pid == pid) {
            return &ptables[i];
        }
    }

    return NULL;
}

inline void ptables_init(void)
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

    inode_put(p->pwd);
    p->pwd = NULL;
    inode_put(p->root);
    p->root = NULL;

    for (s32 fd = 0; fd < MAX_OPEN_FILES; fd += 1) {
        file_t* f = p->open_files[fd];
        if (f) {
            p->open_files[fd] = NULL;

            if (f->f_inode && f->f_op && f->f_op->release) {
                f->f_op->release(f->f_inode, f);
            }

            file_put(f);
        }
    }

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

        waitchan(myproc());
    }
}

void* setup_kvm(void)
{
    u32* pgdir = (u32*)get_free_page();
    if (!pgdir) {
        return NULL;
    }

    for (int i = 0; i < 4; i++) {
        if (page_directory[i] & PTE_P) {
            pgdir[i] = page_directory[i];
        }
    }

    for (int i = KERNBASE << 22; i < 1024; i++) {
        if (page_directory[i] & PTE_P) {
            pgdir[i] = page_directory[i];
        }
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

            u32 kernel_stack_top = (u32)p->kstack + PAGE_SIZE;
            tss_set_stack(kernel_stack_top);

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
