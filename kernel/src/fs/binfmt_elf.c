#include "arch/x86/memlayout.h"
#include "drivers/printk.h"
#include "fs/exec.h"
#include "idt/idt.h"
#include "idt/syscalls.h"
#include "lib/math.h"
#include "memory/consts.h"
#include "memory/page.h"
#include "memory/vmm.h"

#include <ferrite/elf.h>
#include <uapi/errno.h>
#include <ferrite/string.h>

int load_elf_binary(binpgm_t* pgm, trapframe_t* regs)
{
    elf32_hdr_t* elf = (elf32_hdr_t*)pgm->b_buf;

    if (memcmp(elf->e_ident, ELFMAG, SELFMAG) != 0) {
        return -ENOEXEC;
    }

    if (elf->e_type != ET_EXEC) {
        return -ENOEXEC;
    }

    for (int i = 0; i < elf->e_phnum; i++) {
        elf32_phdr_t* phdr = (elf32_phdr_t*)(pgm->b_buf + elf->e_phoff
                                             + i * sizeof(elf32_phdr_t));
        if (phdr->p_type != PT_LOAD) {
            continue;
        }

        u32 addr = phdr->p_vaddr & PAGE_MASK;
        u32 end = ALIGN(phdr->p_vaddr + phdr->p_memsz, PAGE_SIZE);

        for (; addr < end; addr += PAGE_SIZE) {
            if (vmm_map_page(NULL, (void*)addr, PTE_P | PTE_U | PTE_W) < 0) {
                printk("Failed to map page at 0x%x\n", addr);
                return -ENOMEM;
            }
        }

        if (phdr->p_filesz > 0) {
            int ret = read_exec(
                pgm->b_node, phdr->p_offset, (char*)phdr->p_vaddr,
                phdr->p_filesz
            );
            printk("read_exec returned %d\n", ret);
        }

        if (phdr->p_memsz > phdr->p_filesz) {
            memset(
                (char*)phdr->p_vaddr + phdr->p_filesz, 0,
                phdr->p_memsz - phdr->p_filesz
            );
        }
    }

    void* stack_page = get_free_page();
    if (!stack_page) {
        return -ENOMEM;
    }

    // Might be leaking? Double check later
    vmm_remap_page(
        (void*)(KERNBASE - PAGE_SIZE), (void*)V2P_WO((u32)stack_page),
        PTE_P | PTE_U | PTE_W
    );

    regs->eip = elf->e_entry;
    regs->esp = KERNBASE - 16;
    regs->eflags = 0x200;
    regs->cs = USER_CS;
    regs->ss = regs->ds = regs->es = USER_DS;

    return 0;
}
