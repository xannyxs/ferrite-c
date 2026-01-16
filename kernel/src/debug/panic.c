#include "arch/x86/idt/idt.h"
#include "arch/x86/io.h"
#include "drivers/printk.h"

#include <ferrite/types.h>

__attribute__((target("general-regs-only"))) static void
save_stack(u32 stack_pointer)
{
    printk("\n--- STACK DUMP ---\n");
    u32* stack = (u32*)stack_pointer;

    for (int i = 0; i < 16; i++) {
        printk("0x%x ", stack[i]);
    }

    printk("\n------------------\n");
}

__attribute__((target("general-regs-only"))) static void clean_registers(void)
{
    __asm__ volatile("xor %%eax, %%eax\n"
                     "xor %%ebx, %%ebx\n"
                     "xor %%ecx, %%ecx\n"
                     "xor %%edx, %%edx\n"
                     "xor %%esi, %%esi\n"
                     "xor %%edi, %%edi\n"
                     :
                     :
                     : "eax", "ebx", "ecx", "edx", "esi", "edi");
}

__attribute__((target("general-regs-only"), noreturn)) void
panic(trapframe_t* regs, char const* msg)
{
    cli();

    printk("!!! KERNEL PANIC !!!\n");
    printk("%s\n\n", msg);
    printk("EAX: 0x%x\n", regs->eax);
    printk("EIP: 0x%x\n", regs->eip);
    printk("CS:  0x%x\n", regs->cs);
    printk("EFLAGS: 0x%x\n", regs->eflags);

    save_stack((u32)regs);
    clean_registers();

    printk("\nSystem Halted.");

    while (1) {
        cli();
        __asm__ volatile("hlt");
    }
}
