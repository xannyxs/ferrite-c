#include "arch/x86/idt/idt.h"
#include "debug/debug.h"
#include "debug/panic.h"
#include "drivers/printk.h"
#include "memory/vmm.h"
#include "sys/process.h"
#include "types.h"

#define USER_SPACE 1
#define KERNEL_SPACE 3

// TODO: Might need to move this
#define USER_SPACE_START 0x00000000
#define USER_SPACE_END 0xC0000000

__attribute__((target("general-regs-only"), interrupt)) void
divide_by_zero_handler(registers_t* regs)
{
    if ((regs->cs & KERNEL_SPACE) == 0) {
        panic(regs, "Division by Zero");
    }

    __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"), interrupt)) void
debug_interrupt_handler(registers_t* regs)
{
    (void)regs;
    BOCHS_MAGICBREAK();
}

__attribute__((target("general-regs-only"), interrupt)) void
non_maskable_interrupt_handler(registers_t* regs)
{
    panic(regs, "Non Maskable Interrupt");
}

__attribute__((target("general-regs-only"), interrupt)) void
breakpoint_handler(registers_t* regs)
{
    (void)regs;
    BOCHS_MAGICBREAK();
}

__attribute__((target("general-regs-only"), interrupt)) void
overflow_handler(registers_t* regs)
{
    (void)regs;
    // TODO:

    __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"), interrupt)) void
bound_range_exceeded_handler(registers_t* regs)
{
    (void)regs;
    // TODO:

    __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"), interrupt)) void
invalid_opcode(registers_t* regs)
{
    if ((regs->cs & 3) == 0) {
        panic(regs, "Invalid Opcode");
    }

    __asm__ volatile("cli; hlt");
}

// NOTE:
// This exception is primarily used to handle FPU context switching. Without an
// FPU, the CPU won't generate this fault for floating-point instructions.
__attribute__((target("general-regs-only"), interrupt)) void
device_not_available(registers_t* regs)
{
    (void)regs;

    __asm__ volatile("cli; hlt");
}

void x87_fpu_exception(registers_t* regs)
{
    (void)regs;

    __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"), interrupt)) void
reserved_by_cpu(registers_t* regs)
{
    panic(regs, "Reserved by CPU");
}

// --- Handlers that HAVE an error code ---

__attribute__((target("general-regs-only"), interrupt)) void
double_fault(registers_t* regs, u32 error_code)
{
    (void)error_code;
    panic(regs, "Double Fault");
}

__attribute__((target("general-regs-only"), interrupt)) void
invalid_tss(registers_t* regs, u32 error_code)
{
    (void)error_code;

    panic(regs, "Invalid TSS");
}

__attribute__((target("general-regs-only"), interrupt)) void
segment_not_present(registers_t* regs, u32 error_code)
{
    (void)error_code;
    (void)regs;
    // TODO:

    __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"), interrupt)) void
stack_segment_fault(registers_t* regs, u32 error_code)
{
    (void)error_code;
    (void)regs;
    // TODO:

    __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"), interrupt)) void
general_protection_fault(registers_t* regs, u32 error_code)
{
    (void)error_code;
    if ((regs->cs & 3) == 0) {
        printk("GPF: error=%x, cs=%x\n",
            error_code, regs->cs);
        panic(regs, "General Protection Fault");
    }

    printk("GPF: error=%x, cs=%x\n",
        error_code, regs->cs);
    __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"), interrupt)) void
page_fault(registers_t* regs, u32 error_code)
{
    u32 fault_addr;
    __asm__ volatile("movl %%cr2, %0" : "=r"(fault_addr));

    if ((regs->cs & 0x3) == 0x3) {
        proc_t* p = myproc();

        if (fault_addr < USER_SPACE_END) {
            if ((error_code & PTE_P) == 0) {
                void* page_base = (void*)(fault_addr & ~0xFFF);

                if (vmm_map_page(NULL, page_base, PTE_P | PTE_W | PTE_U) == 0) {
                    printk("On-demand mapped page at %p for PID %d\n",
                        page_base, p->pid);
                    return;
                }
            }
        }
    }

    printk("Page Fault: addr=%p, error=%x, cs=%x\n",
        fault_addr, error_code, regs->cs);
    panic(regs, "Page Fault");
    __builtin_unreachable();
}
