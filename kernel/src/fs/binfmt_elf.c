#include "arch/x86/memlayout.h"
#include "drivers/printk.h"
#include "fs/exec.h"
#include "idt/idt.h"
#include "idt/syscalls.h"
#include "lib/math.h"
#include "memory/buddy_allocator/buddy.h"
#include "memory/consts.h"
#include "memory/vmm.h"
#include "sys/process/process.h"

#include <ferrite/elf.h>
#include <ferrite/string.h>
#include <uapi/errno.h>

#define MAX_HEAP_SIZE (128 * 1024 * 1024)
#define MAX_ARGS 10

int load_elf_binary(binpgm_t* pgm, trapframe_t* regs)
{
    unsigned int bss_end = 0;
    elf32_hdr_t* elf = (elf32_hdr_t*)pgm->b_buf;

    if (memcmp(elf->e_ident, ELFMAG, SELFMAG) != 0) {
        return -ENOEXEC;
    }

    if (elf->e_type != ET_EXEC) {
        return -ENOEXEC;
    }

    for (int i = 0; i < elf->e_phnum; i++) {
        elf32_phdr_t* phdr = (elf32_phdr_t*)(pgm->b_buf + elf->e_phoff
                                             + (i * sizeof(elf32_phdr_t)));
        if (phdr->p_type != PT_LOAD) {
            continue;
        }

        u32 addr = phdr->p_vaddr & PAGE_MASK;
        u32 end = ALIGN(phdr->p_vaddr + phdr->p_memsz, PAGE_SIZE);

        for (; addr < end; addr += PAGE_SIZE) {
            void* old_page = vmm_unmap_page((void*)addr);
            if (old_page && buddy_manages((paddr_t)old_page)) {
                buddy_dealloc((paddr_t)old_page, 0);
            }

            if (vmm_map_page(NULL, (void*)addr, PTE_P | PTE_U | PTE_W) < 0) {
                printk("Failed to map page at 0x%x\n", addr);
                return -ENOMEM;
            }
        }

        if (phdr->p_filesz > 0) {
            read_exec(
                pgm->b_node, phdr->p_offset, (char*)phdr->p_vaddr,
                phdr->p_filesz
            );
        }

        if (phdr->p_memsz > phdr->p_filesz) {
            memset(
                (char*)phdr->p_vaddr + phdr->p_filesz, 0,
                phdr->p_memsz - phdr->p_filesz
            );
        }

        unsigned int segment_end = phdr->p_vaddr + phdr->p_memsz;
        if (segment_end > bss_end) {
            bss_end = segment_end;
        }
    }

    myproc()->mm.heap_start = ALIGN(bss_end, PAGE_SIZE);
    myproc()->mm.current = myproc()->mm.heap_start;
    myproc()->mm.heap_end = myproc()->mm.heap_start + MAX_HEAP_SIZE;

    myproc()->mm.stack_start = KERNBASE - (PAGE_SIZE * MAX_ARG_PAGES);

    if (myproc()->mm.heap_end > myproc()->mm.stack_start) {
        myproc()->mm.heap_end = myproc()->mm.stack_start;

        if (myproc()->mm.heap_end <= myproc()->mm.heap_start) {
            printk("Error: No space for heap\n");
            return -ENOMEM;
        }
    }

    for (int i = 0; i < MAX_ARG_PAGES; i++) {
        if (pgm->b_page[i]) {
            u32 vaddr = myproc()->mm.stack_start + (i * PAGE_SIZE);
            u32 paddr = V2P_WO(pgm->b_page[i]);
            vmm_remap_page((void*)vaddr, (void*)paddr, PTE_P | PTE_U | PTE_W);
            pgm->b_page[i] = 0;
        }
    }

    u32 sp = myproc()->mm.stack_start + pgm->b_p;
    char* str_ptr = (char*)sp;
    u32 argv_addrs[MAX_ARGS];
    int argc = pgm->argc;

    for (int i = 0; i < argc; i++) {
        argv_addrs[i] = (u32)str_ptr;
        str_ptr += strlen(str_ptr) + 1;
    }

    int total_entries = 1 + argc + 1;
    int total_bytes = total_entries * 4;

    sp = (myproc()->mm.stack_start + pgm->b_p - total_bytes) & ~0xF;

    u32* stack = (u32*)sp;
    stack[0] = argc;
    for (int i = 0; i < argc; i++) {
        stack[1 + i] = argv_addrs[i];
    }
    stack[1 + argc] = 0;

    regs->esp = sp;
    regs->eip = elf->e_entry;
    regs->eflags = 0x200;
    regs->cs = USER_CS;
    regs->ss = regs->ds = regs->es = USER_DS;

    return 0;
}
