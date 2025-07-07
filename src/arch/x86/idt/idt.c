#include "arch/x86/idt/idt.h"
#include "arch/x86/entry.h"
#include "arch/x86/io.h"
#include "arch/x86/time/rtc.h"

#include <stdint.h>

const interrupt_handler_entry_t INTERRUPT_HANDLERS[21] = {
    {REGULAR, .handler.regular = divide_by_zero_handler},
    {REGULAR, .handler.regular = debug_interrupt_handler},
    {REGULAR, .handler.regular = non_maskable_interrupt_handler},
    {REGULAR, .handler.regular = breakpoint_handler},
    {REGULAR, .handler.regular = overflow_handler},
    {REGULAR, .handler.regular = bound_range_exceeded_handler},
    {REGULAR, .handler.regular = invalid_opcode},
    {REGULAR, .handler.regular = device_not_available},
    {WITH_ERROR_CODE, .handler.with_error_code = double_fault},
    {REGULAR, .handler.regular = coprocessor_segment_overrun},
    {WITH_ERROR_CODE, .handler.with_error_code = invalid_tss},
    {WITH_ERROR_CODE, .handler.with_error_code = segment_not_present},
    {WITH_ERROR_CODE, .handler.with_error_code = stack_segment_fault},
    {WITH_ERROR_CODE, .handler.with_error_code = general_protection_fault},
    {WITH_ERROR_CODE, .handler.with_error_code = page_fault},
    {REGULAR, .handler.regular = x87_floating_point},
    {WITH_ERROR_CODE, .handler.with_error_code = alignment_check},
    {REGULAR, .handler.regular = machine_check},
    {REGULAR, .handler.regular = simd_floating_point},
    {REGULAR, .handler.regular = virtualization},
    {WITH_ERROR_CODE, .handler.with_error_code = security_exception},
};

interrupt_descriptor_t idt_entries[IDT_ENTRY_COUNT];
descriptor_pointer_t idt_ptr;

static void idt_set_gate(uint32_t num, uint32_t handler) {
  idt_entries[num].pointer_low = (handler & 0xffff);
  idt_entries[num].selector = 0x08;
  idt_entries[num].zero = 0;
  idt_entries[num].type_attributes = 0x8E;
  idt_entries[num].pointer_high = ((handler >> 16) & 0xffff);
}

void idt_init(void) {
  for (int32_t i = 0; i < 21; i += 1) {
    uint32_t handler = 0;

    if (INTERRUPT_HANDLERS[i].type == REGULAR) {
      handler = (uint32_t)INTERRUPT_HANDLERS[i].handler.regular;
    } else if (INTERRUPT_HANDLERS[i].type == WITH_ERROR_CODE) {
      handler = (uint32_t)INTERRUPT_HANDLERS[i].handler.with_error_code;
    }

    idt_set_gate(i, handler);
  }

  idt_set_gate(0x21, (uint32_t)keyboard_handler);
  idt_set_gate(0x28, (uint32_t)rtc_handler);

  rtc_init();

  idt_ptr.limit = sizeof(entry_t) * IDT_ENTRY_COUNT - 1;
  idt_ptr.base = (uint32_t)&idt_entries;

  __asm__ __volatile__("lidt %0" : : "m"(idt_ptr));
}
