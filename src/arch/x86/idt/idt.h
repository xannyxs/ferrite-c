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

struct interrupt_frame {
  uint32_t instruction_pointer;
  uint32_t code_segment;
  uint32_t eflags;
  uint32_t stack_pointer;
  uint32_t stack_segment;
};

typedef void (*interrupt_handler)(struct interrupt_frame *frame);
typedef void (*interrupt_handler_with_error)(struct interrupt_frame *frame,
                                             uint32_t error_code);

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
void divide_by_zero_handler(struct interrupt_frame *frame);
void debug_interrupt_handler(struct interrupt_frame *frame);
void non_maskable_interrupt_handler(struct interrupt_frame *frame);
void breakpoint_handler(struct interrupt_frame *frame);
void overflow_handler(struct interrupt_frame *frame);
void bound_range_exceeded_handler(struct interrupt_frame *frame);
void invalid_opcode(struct interrupt_frame *frame);
void device_not_available(struct interrupt_frame *frame);
void coprocessor_segment_overrun(struct interrupt_frame *frame);
void x87_floating_point(struct interrupt_frame *frame);
void machine_check(struct interrupt_frame *frame);
void simd_floating_point(struct interrupt_frame *frame);
void virtualization(struct interrupt_frame *frame);

// --- Handlers that HAVE an error code ---
void double_fault(struct interrupt_frame *frame, uint32_t error_code);
void invalid_tss(struct interrupt_frame *frame, uint32_t error_code);
void segment_not_present(struct interrupt_frame *frame, uint32_t error_code);
void stack_segment_fault(struct interrupt_frame *frame, uint32_t error_code);
void general_protection_fault(struct interrupt_frame *frame,
                              uint32_t error_code);
void page_fault(struct interrupt_frame *frame, uint32_t error_code);
void alignment_check(struct interrupt_frame *frame, uint32_t error_code);
void security_exception(struct interrupt_frame *frame, uint32_t error_code);

// --- Hardware Interrupts ---
void keyboard_handler(struct interrupt_frame *frame);

void idt_init(void);

#endif /* IDT_H */
