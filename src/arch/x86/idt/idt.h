#ifndef IDT_H
#define IDT_H

#include <stdint.h>

#define IDT_ENTRY_COUNT 256

typedef struct interrupt_descriptor {
  uint16_t pointer_low;    // offset bits 0..15
  uint16_t selector;       // a code segment selector in GDT or LDT
  uint8_t zero;            // unused, set to 0
  uint8_t type_attributes; // gate type, dpl, and p fields
  uint16_t pointer_high;   // offset bits 16..31
} interrupt_descriptor_t;

typedef struct registers {
  uint32_t ds;
  uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
  uint32_t int_no, err_code;
  uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

typedef void (*interrupt_handler)(registers_t *);
typedef void (*interrupt_handler_with_error)(registers_t *, uint32_t);

typedef enum {
  REGULAR,
  WITH_ERROR_CODE,
} interrupt_handler_e;

typedef struct {
  interrupt_handler_e type;
  union {
    interrupt_handler regular;
    interrupt_handler_with_error with_error_code;
  } handler;
} interrupt_handler_entry_t;

// --- Handlers that DO NOT have an error code ---
void divide_by_zero_handler(registers_t *);
void debug_interrupt_handler(registers_t *);
void non_maskable_interrupt_handler(registers_t *);
void breakpoint_handler(registers_t *);
void overflow_handler(registers_t *);
void bound_range_exceeded_handler(registers_t *);
void invalid_opcode(registers_t *);
void device_not_available(registers_t *);
void coprocessor_segment_overrun(registers_t *);
void x87_floating_point(registers_t *);
void machine_check(registers_t *);
void simd_floating_point(registers_t *);
void virtualization(registers_t *);

// --- Handlers that HAVE an error code ---
void double_fault(registers_t *, uint32_t error_code);
void invalid_tss(registers_t *, uint32_t error_code);
void segment_not_present(registers_t *, uint32_t error_code);
void stack_segment_fault(registers_t *, uint32_t error_code);
void general_protection_fault(registers_t *, uint32_t error_code);
void page_fault(registers_t *, uint32_t error_code);
void alignment_check(registers_t *, uint32_t error_code);
void security_exception(registers_t *, uint32_t error_code);

// --- Hardware Interrupts ---
void keyboard_handler(registers_t *);
void rtc_handler(registers_t *);

void idt_init(void);

#endif /* IDT_H */
