#include "arch/x86/idt/idt.h"
#include "arch/x86/io.h"
#include "arch/x86/pic.h"
#include "arch/x86/time/rtc.h"
#include "debug/debug.h"
#include "debug/panic.h"
#include "drivers/keyboard.h"
#include "drivers/printk.h"
#include "sys/tasks.h"

#include <stdint.h>

__attribute__((target("general-regs-only"), interrupt)) void
divide_by_zero_handler(registers_t *regs) {
  panic(regs, "Division by Zero");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
debug_interrupt_handler(registers_t *regs) {
  (void)regs;
  BOCHS_BREAK();
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
non_maskable_interrupt_handler(registers_t *regs) {
  (void)regs;

  panic(regs, "Non Maskable Interrupt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
breakpoint_handler(registers_t *regs) {
  (void)regs;
  BOCHS_BREAK();
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
overflow_handler(registers_t *regs) {
  (void)regs;

  panic(regs, "Overflow");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
bound_range_exceeded_handler(registers_t *regs) {
  (void)regs;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
invalid_opcode(registers_t *regs) {
  (void)regs;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
device_not_available(registers_t *regs) {
  (void)regs;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
coprocessor_segment_overrun(registers_t *regs) {
  (void)regs;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
x87_floating_point(registers_t *regs) {
  (void)regs;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
machine_check(registers_t *regs) {
  (void)regs;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
simd_floating_point(registers_t *regs) {
  (void)regs;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
virtualization(registers_t *regs) {
  (void)regs;

  __asm__ volatile("cli; hlt");
}

// --- Handlers that HAVE an error code ---

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
double_fault(registers_t *regs, uint32_t error_code) {
  (void)error_code;
  (void)regs;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
invalid_tss(registers_t *regs, uint32_t error_code) {
  (void)regs;
  (void)error_code;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
segment_not_present(registers_t *regs, uint32_t error_code) {
  (void)regs;
  (void)error_code;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
stack_segment_fault(registers_t *regs, uint32_t error_code) {
  (void)error_code;
  (void)regs;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
general_protection_fault(registers_t *regs, uint32_t error_code) {
  (void)regs;
  (void)error_code;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
page_fault(registers_t *regs, uint32_t error_code) {
  (void)regs;
  (void)error_code;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
alignment_check(registers_t *regs, uint32_t error_code) {
  (void)regs;
  (void)error_code;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
security_exception(registers_t *regs, uint32_t error_code) {
  (void)regs;
  (void)error_code;

  __asm__ volatile("cli; hlt");
}

/* Hardware Interrupts */

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
keyboard_handler(registers_t *regs) {
  (void)regs;

  int32_t scancode = inb(KEYBOARD_DATA_PORT);
  setscancode(scancode);

  schedule_task(keyboard_input);

  pic_send_eoi(1);
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
rtc_handler(registers_t *regs) {
  (void)regs;

  schedule_task(rtc_task);

  pic_send_eoi(8);
}
