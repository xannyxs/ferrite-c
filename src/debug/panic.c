#include "arch/x86/idt/idt.h"
#include "drivers/printk.h"

#include <stdbool.h>

void save_stack(uint32_t stack_pointer) {
  printk("\n--- STACK DUMP ---\n");
  uint32_t *stack = (uint32_t *)stack_pointer;
  for (int i = 0; i < 16; i++) {
    printk("0x%x ", stack[i]);
  }
  printk("\n------------------\n");
}

void clean_registers(void) {
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

__attribute__((__noreturn__)) void panic(registers_t *regs, const char *msg) {
  __asm__ volatile("cli");

  printk("!!! KERNEL PANIC !!!\n");
  printk("%s\n\n", msg);

  printk("EAX: 0x%x\n", regs->eax);
  printk("EIP: 0x%x\n", regs->eip);
  printk("CS:  0x%x\n", regs->cs);
  printk("EFLAGS: 0x%x\n", regs->eflags);
  printk("INT: 0x%x\n", regs->int_no);

  save_stack((uint32_t)regs);

  clean_registers();

  printk("\nSystem Halted.");
  while (true) {
    __asm__ volatile("hlt");
  }
  __builtin_unreachable();
}
