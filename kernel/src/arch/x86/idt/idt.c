#include "arch/x86/idt/idt.h"
#include "arch/x86/entry.h"

extern void syscall_handler(trapframe_t*);
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

extern void irq_stub_0(void);
extern void irq_stub_1(void);
extern void irq_stub_7(void);

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

interrupt_hardware_t const HARDWARE_HANDLERS[NUM_HARDWARE_HANDLERS]
    = { { 0x20, irq_stub_0 }, { 0x21, irq_stub_1 }, { 0x27, irq_stub_7 } };

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
        idt_set_gate(
            HARDWARE_HANDLERS[i].hex, (u32)HARDWARE_HANDLERS[i].f, 0x8E
        );
    }

    // Syscalls
    idt_set_gate(0x80, (u32)syscall_handler, 0xEE);

    idt_ptr.limit = (sizeof(entry_t) * IDT_ENTRY_COUNT) - 1;
    idt_ptr.base = (u32)&idt_entries;

    __asm__ __volatile__("lidt %0" : : "m"(idt_ptr));
}
