#include "arch/x86/idt/idt.h"
#include "drivers/video/vga.h"

#include <stdbool.h>

__attribute__((target("general-regs-only"))) void
save_stack(uint32_t stack_pointer) {
  vga_writestring("\n--- STACK DUMP ---\n");
  uint32_t *stack = (uint32_t *)stack_pointer;
  for (int i = 0; i < 16; i++) {
    vga_write_hex(stack[i]);
    vga_putchar(' ');
  }
  vga_writestring("\n------------------\n");
}

__attribute__((target("general-regs-only"))) void clean_registers(void) {
  __asm__ volatile("xor %%eax, %%eax\n"
                   "xor %%ebx, %%ebx\n"
                   "xor %%ecx, %%ecx\n"
                   "xor %%edx, %%edx\n"
                   "xor %%esi, %%esi\n"
                   "xor %%edi, %%edi\n"
                   :
                   :
                   : "eax", "ebx", "ecx", "edx", "esi", "edi");
}

__attribute__((target("general-regs-only"), noreturn)) void
panic(registers_t *regs, const char *msg) {
  (void)msg;
  __asm__ volatile("cli");

  vga_writestring("!!! KERNEL PANIC !!!\n");
  vga_writestring(msg);
  vga_writestring("\n\n");

  vga_writestring("EAX: ");
  vga_write_hex(regs->eax);
  vga_putchar('\n');
  vga_writestring("EIP: ");
  vga_write_hex(regs->eip);
  vga_putchar('\n');
  vga_writestring("CS:  ");
  vga_write_hex(regs->cs);
  vga_putchar('\n');
  vga_writestring("EFLAGS: ");
  vga_write_hex(regs->eflags);
  vga_putchar('\n');
  vga_writestring("INT: ");
  vga_write_hex(regs->int_no);
  vga_putchar('\n');

  save_stack((uint32_t)regs);
  clean_registers();

  vga_writestring("\nSystem Halted.");
  while (true) {
    __asm__ volatile("hlt");
  }
}
