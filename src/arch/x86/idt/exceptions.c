#include "arch/x86/idt/idt.h"
#include "debug/debug.h"
#include "debug/panic.h"
#include "drivers/printk.h"
#include "sys/process.h"
#include "types.h"

#define KERNEL_MODE 0
#define USER_MODE 3

typedef void (*exception_handler_t)(registers_t*, u32);
exception_handler_t const EXCEPTION_HANDLERS[NUM_EXCEPTION_HANDLERS] = {
    divide_by_zero_handler,
    debug_interrupt_handler,
    non_maskable_interrupt_handler,
    breakpoint_handler,
    overflow_handler,
    bound_range_exceeded_handler,
    invalid_opcode,
    device_not_available,
    double_fault,
    reserved_by_cpu,
    invalid_tss,
    segment_not_present,
    stack_segment_fault,
    general_protection_fault,
    page_fault,
    reserved_by_cpu,
    x87_fpu_exception,
};

void exception_dispatcher_c(registers_t* reg)
{
    if (reg->int_no >= NUM_EXCEPTION_HANDLERS) {
        printk("Unknown exception: %d\n", reg->int_no);
        return;
    }

    EXCEPTION_HANDLERS[reg->int_no](reg, reg->err_code);
}

__attribute__((target("general-regs-only"))) void
divide_by_zero_handler(registers_t* regs, u32 error_code)
{
    (void)error_code;
    if ((regs->cs & 3) == KERNEL_MODE) {
        panic(regs, "Division by Zero");
    }

    do_exit(-1);
}

__attribute__((target("general-regs-only"))) void
debug_interrupt_handler(registers_t* regs, u32 error_code)
{
    (void)error_code;
    (void)regs;

    printk("Debug interrupt");
    BOCHS_MAGICBREAK();
}

__attribute__((target("general-regs-only"))) void
non_maskable_interrupt_handler(registers_t* regs, u32 error_code)
{
    (void)error_code;
    panic(regs, "Non Maskable Interrupt");
}

__attribute__((target("general-regs-only"))) void
breakpoint_handler(registers_t* regs, u32 error_code)
{
    (void)error_code;
    (void)regs;
    BOCHS_MAGICBREAK();
}

__attribute__((target("general-regs-only"))) void
overflow_handler(registers_t* regs, u32 error_code)
{
    (void)regs;
    (void)error_code;
    // TODO:

    __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) void
bound_range_exceeded_handler(registers_t* regs, u32 error_code)
{
    (void)regs;
    (void)error_code;
    // TODO:

    __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) void
invalid_opcode(registers_t* regs, u32 error_code)
{
    (void)error_code;
    if ((regs->cs & 3) == 0) {
        panic(regs, "Invalid Opcode");
    }

    __asm__ volatile("cli; hlt");
}

// NOTE:
// This exception is primarily used to handle FPU context switching. Without an
// FPU, the CPU won't generate this fault for floating-point instructions.
__attribute__((target("general-regs-only"))) void
device_not_available(registers_t* regs, u32 error_code)
{
    (void)error_code;
    (void)regs;

    __asm__ volatile("cli; hlt");
}

void x87_fpu_exception(registers_t* regs, u32 error_code)
{
    (void)regs;
    (void)error_code;

    __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) void
reserved_by_cpu(registers_t* regs, u32 error_code)
{
    (void)error_code;
    (void)regs;
}

// --- Handlers that HAVE an error code ---

__attribute__((target("general-regs-only"))) void
double_fault(registers_t* regs, u32 error_code)
{
    (void)error_code;
    (void)error_code;
    panic(regs, "Double Fault");
}

__attribute__((target("general-regs-only"))) void
invalid_tss(registers_t* regs, u32 error_code)
{
    (void)error_code;
    (void)error_code;

    panic(regs, "Invalid TSS");
}

__attribute__((target("general-regs-only"))) void
segment_not_present(registers_t* regs, u32 error_code)
{
    (void)error_code;
    (void)regs;
    // TODO:

    __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) void
stack_segment_fault(registers_t* regs, u32 error_code)
{
    (void)error_code;
    (void)regs;
    // TODO:

    __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) void
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

__attribute__((target("general-regs-only"))) void
page_fault(registers_t* regs, u32 error_code)
{
    (void)error_code;
    if ((regs->cs & 3) == KERNEL_MODE) {
        panic(regs, "Page Fault");
    }

    printk("Page Fault: addr=%p, error=%x, cs=%x\n",
        fault_addr, error_code, regs->cs);
    panic(regs, "Page Fault");
    __builtin_unreachable();
}
