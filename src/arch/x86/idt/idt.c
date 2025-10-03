#include "arch/x86/idt/idt.h"
#include "arch/x86/entry.h"
#include "drivers/printk.h"

#define NUM_EXCEPTION_HANDLERS 17
#define NUM_HARDWARE_HANDLERS 3

extern void syscall_handler(registers_t*);

extern void divide_by_zero_stub(void);
extern void debug_interrupt_stub(void);
extern void non_maskable_interrupt_stub(void);
extern void breakpoint_stub(void);
extern void overflow_stub(void);
extern void bound_range_exceeded_stub(void);
extern void invalid_opcode_stub(void);
extern void device_not_available_stub(void);
extern void double_fault_stub(void);
extern void invalid_tss_stub(void);
extern void segment_not_present_stub(void);
extern void stack_segment_fault_stub(void);
extern void general_protection_fault_stub(void);
extern void page_fault_stub(void);
extern void x87_fpu_exception_stub(void);
extern void reserved(void);

interrupt_descriptor_t idt_entries[IDT_ENTRY_COUNT];
descriptor_pointer_t idt_ptr;

void (*const EXCEPTION_STUBS[NUM_EXCEPTION_HANDLERS])(void) = {
    divide_by_zero_stub,
    debug_interrupt_stub,
    non_maskable_interrupt_stub,
    breakpoint_stub,
    overflow_stub,
    bound_range_exceeded_stub,
    invalid_opcode_stub,
    device_not_available_stub,
    double_fault_stub,
    reserved,
    invalid_tss_stub,
    segment_not_present_stub,
    stack_segment_fault_stub,
    general_protection_fault_stub,
    page_fault_stub,
    reserved,
    x87_fpu_exception_stub,
};

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

interrupt_hardware_t const HARDWARE_HANDLERS[NUM_HARDWARE_HANDLERS] = {
    { 0x20, timer_handler }, { 0x21, keyboard_handler }, { 0x27, spurious_handler }
};

static void idt_set_gate(u32 num, u32 handler, u32 attributes)
{
    idt_entries[num].pointer_low = (handler & 0xffff);
    idt_entries[num].selector = 0x08;
    idt_entries[num].zero = 0;
    idt_entries[num].type_attributes = attributes;
    idt_entries[num].pointer_high = ((handler >> 16) & 0xffff);
}

void idt_init(void)
{
    // Exceptions
    for (s32 i = 0; i < NUM_EXCEPTION_HANDLERS; i += 1) {
        u32 handler = (u32)EXCEPTION_STUBS[i];
        idt_set_gate(i, handler, 0x8E);
    }

    // Hardware Interrupts
    for (s32 i = 0; i < NUM_HARDWARE_HANDLERS; i += 1) {
        idt_set_gate(HARDWARE_HANDLERS[i].hex, (u32)HARDWARE_HANDLERS[i].func, 0x8E);
    }

    // Syscalls
    idt_set_gate(0x80, (u32)syscall_handler, 0xEE);

    idt_ptr.limit = sizeof(entry_t) * IDT_ENTRY_COUNT - 1;
    idt_ptr.base = (u32)&idt_entries;

    __asm__ __volatile__("lidt %0" : : "m"(idt_ptr));
}
