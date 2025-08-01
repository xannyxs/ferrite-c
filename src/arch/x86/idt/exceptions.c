#include "arch/x86/idt/idt.h"
#include "arch/x86/io.h"
#include "arch/x86/pic.h"
#include "arch/x86/time/rtc.h"
#include "debug/debug.h"
#include "debug/panic.h"
#include "drivers/keyboard.h"
#include "sys/tasks.h"

#include <stdint.h>

__attribute__((target("general-regs-only"), interrupt)) void
divide_by_zero_handler(registers_t *regs) {
  if ((regs->cs & 3) == 0) {
    panic(regs, "Division by Zero");
  }

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"), interrupt)) void
debug_interrupt_handler(registers_t *regs) {
  (void)regs;
  BOCHS_BREAK();
}

__attribute__((target("general-regs-only"), interrupt)) void
non_maskable_interrupt_handler(registers_t *regs) {
  panic(regs, "Non Maskable Interrupt");
}

__attribute__((target("general-regs-only"), interrupt)) void
breakpoint_handler(registers_t *regs) {
  (void)regs;
  BOCHS_BREAK();
}

__attribute__((target("general-regs-only"), interrupt)) void
overflow_handler(registers_t *regs) {
  (void)regs;
  // TODO:

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"), interrupt)) void
bound_range_exceeded_handler(registers_t *regs) {
  (void)regs;
  // TODO:

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"), interrupt)) void
invalid_opcode(registers_t *regs) {
  if ((regs->cs & 3) == 0) {
    panic(regs, "Invalid Opcode");
  }

  __asm__ volatile("cli; hlt");
}

// NOTE:
// This exception is primarily used to handle FPU context switching. Without an
// FPU, the CPU won't generate this fault for floating-point instructions.
__attribute__((target("general-regs-only"), interrupt)) void
device_not_available(registers_t *regs) {
  (void)regs;

  __asm__ volatile("cli; hlt");
}

void x87_fpu_exception(registers_t *regs) {
  (void)regs;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"), interrupt)) void
reserved_by_cpu(registers_t *regs) {
  panic(regs, "Reserved by CPU");
}

// --- Handlers that HAVE an error code ---

__attribute__((target("general-regs-only"), interrupt)) void
double_fault(registers_t *regs, uint32_t error_code) {
  (void)error_code;
  panic(regs, "Double Fault");
}

__attribute__((target("general-regs-only"), interrupt)) void
invalid_tss(registers_t *regs, uint32_t error_code) {
  (void)error_code;

  panic(regs, "Invalid TSS");
}

__attribute__((target("general-regs-only"), interrupt)) void
segment_not_present(registers_t *regs, uint32_t error_code) {
  (void)error_code;
  (void)regs;
  // TODO:

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"), interrupt)) void
stack_segment_fault(registers_t *regs, uint32_t error_code) {
  (void)error_code;
  (void)regs;
  // TODO:

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"), interrupt)) void
general_protection_fault(registers_t *regs, uint32_t error_code) {
  (void)error_code;
  if ((regs->cs & 3) == 0) {
    panic(regs, "General Protection Fault");
  }

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"), interrupt)) void
page_fault(registers_t *regs, uint32_t error_code) {
  (void)error_code;
  if ((regs->cs & 3) == 0) {
    panic(regs, "Page Fault");
  }

  __asm__ volatile("cli; hlt");
}

/* Hardware Interrupts */

__attribute__((target("general-regs-only"), interrupt)) void
keyboard_handler(registers_t *regs) {
  (void)regs;

  int32_t scancode = inb(KEYBOARD_DATA_PORT);
  setscancode(scancode);

  schedule_task(keyboard_input);

  pic_send_eoi(1);
}

__attribute__((target("general-regs-only"), interrupt)) void
rtc_handler(registers_t *regs) {
  (void)regs;

  schedule_task(rtc_task);

  pic_send_eoi(8);
}
