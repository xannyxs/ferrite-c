#include "drivers/console.h"
#include "arch/x86/cpu.h"
#include "arch/x86/entry.h"
#include "arch/x86/time/rtc.h"
#include "arch/x86/time/time.h"
#include "drivers/printk.h"
#include "drivers/video/vga.h"
#include "stdlib.h"

#include <stdint.h>
#include <string.h>

static const char *prompt = "[42]$ ";
static char buffer[VGA_WIDTH];
static int32_t i = 0;

/* Private */

static void delete_character(void) {
  if (i == 0) {
    return;
  }

  buffer[i] = 0;
  i -= 1;
  vga_clear_char();
}

static void print_help(void) {
  printk("Available commands:\n");
  printk("  reboot  - Restart the system\n");
  printk("  gdt     - Print Global Descriptor Table\n");
  printk("  idt     - Print Interrupt Descriptor Table\n");
  printk("  clear   - Clear the screen\n");
  printk("  time    - See the current time\n");
  printk("  epoch   - See the current time since Epoch\n");
  printk("  help    - Show this help message\n");
}

static void print_idt(void) {
  descriptor_pointer_t idtr;

  __asm__ volatile("sidt %0" : "=m"(idtr));

  printk("IDT Base Address: 0x%x\n", idtr.base);
  printk("IDT Limit: 0x%xx\n", idtr.limit);
}

static void print_time(void) {
  rtc_time_t t;
  gettime(&t);

  // print the time in a standard format (e.g., hh:mm:ss dd/mm/yy)
  printk("current time: %u:%u:%u %u/%u/%u\n", t.hour, t.minute, t.second, t.day,
         t.month, t.year);
}

static void print_epoch(void) {
  time_t t = getepoch();

  printk("current Epoch Time: %u\n", t);
}

static void print_gdt(void) {
  descriptor_pointer_t gdtr;

  __asm__ volatile("sgdt %0" : "=m"(gdtr));

  printk("GDT Base Address: 0x%x\n", gdtr.base);
  printk("GDT Limit: 0x%x\n", gdtr.limit);
}

static void execute_buffer(void) {
  static const exec_t command_table[] = {
      {"reboot", reboot},   {"gdt", print_gdt},     {"clear", vga_init},
      {"help", print_help}, {"panic", abort},       {"idt", print_idt},
      {"time", print_time}, {"epoch", print_epoch}, {NULL, NULL}};

  printk("\n");

  for (int32_t i = 0; command_table[i].cmd; i++) {
    if (command_table[i].f && strcmp(buffer, command_table[i].cmd) == 0) {
      command_table[i].f();
      break;
    }
  }

  memset(buffer, 0, VGA_WIDTH);
  i = 0;

  printk("%s", prompt);
}

/* Public */

void console_init(void) { printk("%s", prompt); }

void console_add_buffer(char c) {
  switch (c) {
  case '\n':
    execute_buffer();
    break;
  case '\x08':
    delete_character();
    break;
  default:
    if (i < VGA_WIDTH - 1) {
      buffer[i] = c;
      printk("%c", c);
      i++;
    }
  }
}
