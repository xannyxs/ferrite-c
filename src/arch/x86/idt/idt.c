#include "arch/x86/idt/idt.h"
#include "arch/x86/entry.h"

#include <stdint.h>

#define NUM_EXCEPTION_HANDLERS 17
#define NUM_HARDWARE_HANDLERS 3

extern void syscall_handler(registers_t *);

interrupt_descriptor_t idt_entries[IDT_ENTRY_COUNT];
descriptor_pointer_t idt_ptr;

const interrupt_handler_entry_t EXCEPTION_HANDLERS[NUM_EXCEPTION_HANDLERS] = {
    {REGULAR, .handler.regular = divide_by_zero_handler},
    {REGULAR, .handler.regular = debug_interrupt_handler},
    {REGULAR, .handler.regular = non_maskable_interrupt_handler},
    {REGULAR, .handler.regular = breakpoint_handler},
    {REGULAR, .handler.regular = overflow_handler},
    {REGULAR, .handler.regular = bound_range_exceeded_handler},
    {REGULAR, .handler.regular = invalid_opcode},
    {REGULAR, .handler.regular = device_not_available},
    {WITH_ERROR_CODE, .handler.with_error_code = double_fault},
    {REGULAR, .handler.regular = reserved_by_cpu},
    {WITH_ERROR_CODE, .handler.with_error_code = invalid_tss},
    {WITH_ERROR_CODE, .handler.with_error_code = segment_not_present},
    {WITH_ERROR_CODE, .handler.with_error_code = stack_segment_fault},
    {WITH_ERROR_CODE, .handler.with_error_code = general_protection_fault},
    {WITH_ERROR_CODE, .handler.with_error_code = page_fault},
    {REGULAR, .handler.regular = reserved_by_cpu},
    {REGULAR, .handler.regular = x87_fpu_exception},
};
const interrupt_hardware_t HARDWARE_HANDLERS[NUM_HARDWARE_HANDLERS] = {
    {0x20, timer_handler}, {0x21, keyboard_handler}, {0x27, spurious_handler}};

static void idt_set_gate(uint32_t num, uint32_t handler) {
  idt_entries[num].pointer_low = (handler & 0xffff);
  idt_entries[num].selector = 0x08;
  idt_entries[num].zero = 0;
  idt_entries[num].type_attributes = 0x8E;
  idt_entries[num].pointer_high = ((handler >> 16) & 0xffff);
}

void idt_init(void) {
  // Exceptions
  for (int32_t i = 0; i < NUM_EXCEPTION_HANDLERS; i += 1) {
    uint32_t handler = 0;

    if (EXCEPTION_HANDLERS[i].type == REGULAR) {
      handler = (uint32_t)EXCEPTION_HANDLERS[i].handler.regular;
    } else if (EXCEPTION_HANDLERS[i].type == WITH_ERROR_CODE) {
      handler = (uint32_t)EXCEPTION_HANDLERS[i].handler.with_error_code;
    }

    idt_set_gate(i, handler);
  }

  // Hardware Interrupts
  for (int32_t i = 0; i < NUM_HARDWARE_HANDLERS; i += 1) {
    idt_set_gate(HARDWARE_HANDLERS[i].hex, (uint32_t)HARDWARE_HANDLERS[i].func);
  }

  // Syscalls
  idt_set_gate(0x80, (uint32_t)syscall_handler);

  idt_ptr.limit = sizeof(entry_t) * IDT_ENTRY_COUNT - 1;
  idt_ptr.base = (uint32_t)&idt_entries;

  __asm__ __volatile__("lidt %0" : : "m"(idt_ptr));
}
