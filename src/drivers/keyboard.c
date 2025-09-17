#include "drivers/keyboard.h"
#include "arch/x86/idt/idt.h"
#include "drivers/console.h"
#include "drivers/printk.h"
#include "sys/process.h"

#include <stdbool.h>
#include <stdint.h>

extern struct {
  uint8_t buf[256];
  int32_t head; // Read position
  int32_t tail; // Write position
  pid_t shell_pid;
} tty;

/* Private */

char scancode_to_ascii(uint8_t scan_code, bool shift_pressed) {
  static const char scancode_map_no_shift[] = {
      0,   0,   '1',  '2',  '3',  '4', '5', '6',  '7', '8', '9', '0',
      '-', '=', '\b', '\t', 'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
      'o', 'p', '[',  ']',  '\n', 0,   'a', 's',  'd', 'f', 'g', 'h',
      'j', 'k', 'l',  ';',  '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
      'b', 'n', 'm',  ',',  '.',  '/', 0,   '*',  0,   ' ', 0};

  static const char scancode_map_shift[] = {
      0,   0,   '!',  '@',  '#',  '$', '%', '^', '&', '*', '(', ')',
      '_', '+', '\b', '\t', 'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I',
      'O', 'P', '{',  '}',  '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H',
      'J', 'K', 'L',  ':',  '"',  '~', 0,   '|', 'Z', 'X', 'C', 'V',
      'B', 'N', 'M',  '<',  '>',  '?', 0,   '*', 0,   ' ', 0};

  if (scan_code & 0x80) {
    return 0;
  }

  if (scan_code >= sizeof(scancode_map_no_shift)) {
    return 0;
  }

  if (shift_pressed) {
    return scancode_map_shift[scan_code];
  }

  return scancode_map_no_shift[scan_code];
}

/* Public */

void keyboard_put(uint8_t scancode) {
  static bool SHIFT_PRESSED = false;

  switch (scancode) {
  case 42:
    SHIFT_PRESSED = true;
    return;
  case 170:
    SHIFT_PRESSED = false;
    return;
  }

  char c = scancode_to_ascii(scancode, SHIFT_PRESSED);
  if (!c)
    return;

  console_add_buffer(c);
}
