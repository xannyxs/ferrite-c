#ifndef IDT_H
#define IDT_H

#include "types.h"

#define IDT_ENTRY_COUNT 256
#define NUM_EXCEPTION_HANDLERS 17
#define NUM_HARDWARE_HANDLERS 3

typedef struct interrupt_descriptor {
    u16 pointer_low;    // offset bits 0..15
    u16 selector;       // a code segment selector in GDT or LDT
    u8 zero;            // unused, set to 0
    u8 type_attributes; // gate type, dpl, and p fields
    u16 pointer_high;   // offset bits 16..31
} interrupt_descriptor_t;

typedef struct registers {
    u32 eax, ecx, edx, ebx, esp, ebp, esi, edi;
    u32 gs, fs, es, ds;
    u32 int_no, err_code;
    u32 eip, cs, eflags, useresp, ss;
} registers_t;

typedef struct {
    u32 hex;
    void (*f)(void);
} interrupt_hardware_t;

// --- Handlers that DO NOT have an error code ---
void divide_by_zero_handler(registers_t*, u32);
void debug_interrupt_handler(registers_t*, u32);
void non_maskable_interrupt_handler(registers_t*, u32);
void breakpoint_handler(registers_t*, u32);
void overflow_handler(registers_t*, u32);
void bound_range_exceeded_handler(registers_t*, u32);
void invalid_opcode(registers_t*, u32);
void device_not_available(registers_t*, u32);
void x87_fpu_exception(registers_t*, u32);

// Reserved, does nothing
void reserved_by_cpu(registers_t*, u32);

// --- Handlers that HAVE an error code ---
void double_fault(registers_t*, u32 error_code);
void invalid_tss(registers_t*, u32 error_code);
void segment_not_present(registers_t*, u32 error_code);
void stack_segment_fault(registers_t*, u32 error_code);
void general_protection_fault(registers_t*, u32 error_code);
void page_fault(registers_t*, u32 error_code);

// --- Hardware Interrupts ---
void timer_handler(registers_t*);
void keyboard_handler(registers_t*);
void spurious_handler(registers_t*);

void idt_init(void);

#endif /* IDT_H */
