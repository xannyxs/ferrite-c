#include "arch/x86/idt/idt.h"
#include "arch/x86/io.h"
#include "arch/x86/pic.h"
#include "arch/x86/time/rtc.h"
#include "debug/debug.h"
#include "drivers/console.h"
#include "drivers/keyboard.h"
#include "drivers/printk.h"
#include "lib/stdlib.h"

#include <stdint.h>

void print_frame(struct interrupt_frame *frame) {
  printk("INTERRUPT FRAME:\n");
  printk("  EIP: 0x%x\n", frame->instruction_pointer);
  printk("  CS:  0x%x\n", frame->code_segment);
  printk("  EFLAGS: 0x%x\n", frame->eflags);
  printk("  ESP: 0x%x\n", frame->stack_pointer);
  printk("  SS:  0x%x\n", frame->stack_segment);
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
divide_by_zero_handler(struct interrupt_frame *frame) {
  printk("EXCEPTION: DIVIDE BY ZERO (#DE)\n");

  if ((frame->code_segment & 0x3) == 0) {
    abort("Cannot divide by zero in kernel mode\n");
  }

  printk("User process attempted division by zero\n");
  printk("Terminating process...\n");

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
debug_interrupt_handler(struct interrupt_frame *frame) {
  (void)frame;

  printk("EXCEPTION: DEBUG EXCEPTION (#DB)\n");
  BOCHS_BREAK();
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
non_maskable_interrupt_handler(struct interrupt_frame *frame) {
  (void)frame;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
breakpoint_handler(struct interrupt_frame *frame) {
  (void)frame;

  BOCHS_BREAK();
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
overflow_handler(struct interrupt_frame *frame) {
  (void)frame;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
bound_range_exceeded_handler(struct interrupt_frame *frame) {
  (void)frame;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
invalid_opcode(struct interrupt_frame *frame) {
  (void)frame;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
device_not_available(struct interrupt_frame *frame) {
  (void)frame;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
coprocessor_segment_overrun(struct interrupt_frame *frame) {
  (void)frame;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
x87_floating_point(struct interrupt_frame *frame) {
  (void)frame;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
machine_check(struct interrupt_frame *frame) {
  (void)frame;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
simd_floating_point(struct interrupt_frame *frame) {
  (void)frame;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
virtualization(struct interrupt_frame *frame) {
  (void)frame;

  __asm__ volatile("cli; hlt");
}

// --- Handlers that HAVE an error code ---

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
double_fault(struct interrupt_frame *frame, uint32_t error_code) {
  (void)error_code;
  (void)frame;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
invalid_tss(struct interrupt_frame *frame, uint32_t error_code) {
  (void)frame;
  (void)error_code;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
segment_not_present(struct interrupt_frame *frame, uint32_t error_code) {
  (void)frame;
  (void)error_code;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
stack_segment_fault(struct interrupt_frame *frame, uint32_t error_code) {
  (void)error_code;
  (void)frame;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
general_protection_fault(struct interrupt_frame *frame, uint32_t error_code) {
  (void)frame;
  (void)error_code;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
page_fault(struct interrupt_frame *frame, uint32_t error_code) {
  (void)frame;
  (void)error_code;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
alignment_check(struct interrupt_frame *frame, uint32_t error_code) {
  (void)frame;
  (void)error_code;

  __asm__ volatile("cli; hlt");
}

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
security_exception(struct interrupt_frame *frame, uint32_t error_code) {
  (void)frame;
  (void)error_code;

  __asm__ volatile("cli; hlt");
}

/* Hardware Interrupts */

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
keyboard_handler(struct interrupt_frame *frame) {
  (void)frame;

  int32_t scancode = inb(0x60);

  char c = keyboard_input(scancode);
  if (c != 0) {
    console_add_buffer(c);
  }

  pic_send_eoi(1);
}

volatile uint64_t ticks = 0;

__attribute__((target("general-regs-only"))) __attribute__((interrupt)) void
rtc_handler(struct interrupt_frame *frame) {
  (void)frame;

  outb(0x70, 0x0C); // select register C
  inb(0x71);        // just throw away contents

  ticks += 1;
  int32_t frequency = getfrequency();
  if (ticks % frequency == 0) {
    // seconds_since_epoch++;
  }

  pic_send_eoi(8);
}
