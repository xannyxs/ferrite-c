#include "drivers/keyboard.h"
#include "arch/x86/io.h"

#include <stdbool.h>
#include <stdint.h>

const int16_t KEYBOARD_DATA_PORT = 0x60;
const int16_t KEYBOARD_STATUS_PORT = 0x64;

static bool SHIFT_PRESSED = false;

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

char keyboard_input(uint8_t scancode) {
  // Shift Pressed
  if (scancode == 42) {
    SHIFT_PRESSED = true;
    return 0;
  }

  // Shift Released
  if (scancode == 170) {
    SHIFT_PRESSED = false;
    return 0;
  }

  char c = scancode_to_ascii(scancode, SHIFT_PRESSED);

  return c;
}
