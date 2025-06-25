#include "drivers/console.h"
#include "arch/x86/cpu.h"
#include "drivers/printk.h"
#include "drivers/video/vga.h"
#include "stdlib.h"

#include <stdint.h>
#include <string.h>

static const char *prompt = "[42]$ ";
static char buffer[VGA_WIDTH];
static int32_t i = 0;

/* Private */

void delete_character(void) {
  if (i == 0) {
    return;
  }

  buffer[i] = 0;
  i -= 1;
  vga_clear_char();
}

void print_help() {
  printk("Available commands:\n");
  printk("  reboot  - Restart the system\n");
  printk("  gdt     - Print Global Descriptor Table\n");
  printk("  clear   - Clear the screen\n");
  printk("  help    - Show this help message\n");
}

void execute_buffer(void) {
  static const exec_t command_table[] = {
      {"reboot", reboot},   {"gdt", NULL},    {"clear", vga_init},
      {"help", print_help}, {"panic", abort}, {"idt", NULL},
      {NULL, NULL}};

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
