#include "syscalls.h"
#include "arch/x86/idt/idt.h"
#include "drivers/printk.h"
#include "sys/process.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern volatile bool need_resched;

// TODO: Make sys_exit function
__attribute__((target("general-regs-only"))) static uint32_t
sys_exit(int32_t status) {
  printk("Status: %d\n", status);

  __asm__ volatile("hlt");
  return 0;
}

// TODO: Make _read function
__attribute__((target("general-regs-only"), warn_unused_result)) static int32_t
sys_read(int32_t fd, void *buf, size_t count) {
  (void)fd;
  (void)buf;
  (void)count;

  printk("Read\n");
  return 0;
}

// TODO: Make _write function
__attribute__((target("general-regs-only"), warn_unused_result)) static int32_t
sys_write(int32_t fd, void *buf, size_t count) {
  (void)fd;
  (void)buf;
  (void)count;

  printk("Write\n");
  return 0;
}

__attribute__((target("general-regs-only"))) void
syscall_dispatcher_c(registers_t *reg) {
  switch (reg->eax) {
  case EXIT:
    sys_exit(reg->ebx);
    break;

  case WRITE:
    reg->eax = sys_write(reg->ebx, (void *)reg->ecx, reg->edx);
    break;

  case READ:
    reg->eax = sys_read(reg->ebx, (void *)reg->ecx, reg->edx);
    break;

  default:
    printk("Nothing...?\n");
    break;
  }

  check_resched();
}
