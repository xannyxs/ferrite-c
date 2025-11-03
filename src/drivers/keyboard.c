#include "drivers/console.h"
#include "sys/process/process.h"
#include "types.h"

#include <stdbool.h>

static bool SHIFT_PRESSED = false;
static bool CTRL_PRESSED = false;

extern struct {
    u8 buf[256];
    s32 head; // Read position
    s32 tail; // Write position
    pid_t shell_pid;
} tty;

/* Private */

char scancode_to_ascii(u8 scan_code)
{
    static char const scancode_map_no_shift[]
        = { 0,   0,   '1',  '2',  '3',  '4', '5', '6',  '7', '8', '9', '0',
            '-', '=', '\b', '\t', 'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
            'o', 'p', '[',  ']',  '\n', 0,   'a', 's',  'd', 'f', 'g', 'h',
            'j', 'k', 'l',  ';',  '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
            'b', 'n', 'm',  ',',  '.',  '/', 0,   '*',  0,   ' ', 0 };

    static char const scancode_map_shift[]
        = { 0,   0,   '!',  '@',  '#',  '$', '%', '^', '&', '*', '(', ')',
            '_', '+', '\b', '\t', 'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I',
            'O', 'P', '{',  '}',  '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H',
            'J', 'K', 'L',  ':',  '"',  '~', 0,   '|', 'Z', 'X', 'C', 'V',
            'B', 'N', 'M',  '<',  '>',  '?', 0,   '*', 0,   ' ', 0 };

    static char const scancode_map_ctrl[]
        = { 0,    0,  0,  0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    '\b',
            '\t', 17, 23, 5, 18, 20, 25, 21, 9,  15, 16, 27, 29, '\n', 0,
            1,    19, 4,  6, 7,  8,  10, 11, 12, 0,  0,  0,  0,  28,   26,
            24,   3,  22, 2, 14, 13, 0,  0,  0,  0,  0,  0,  0,  0 };

    if (scan_code & 0x80) {
        return 0;
    }

    if (scan_code >= sizeof(scancode_map_no_shift)) {
        return 0;
    }

    if (CTRL_PRESSED) {
        return scancode_map_ctrl[scan_code];
    }

    if (SHIFT_PRESSED) {
        return scancode_map_shift[scan_code];
    }

    return scancode_map_no_shift[scan_code];
}

/* Public */

void keyboard_put(u8 scancode)
{
    switch (scancode) {
    case 29:
        CTRL_PRESSED = true;
        return;
    case 42:
        SHIFT_PRESSED = true;
        return;
    case 156:
        CTRL_PRESSED = false;
        return;
    case 170:
        SHIFT_PRESSED = false;
        return;
    default:
        break;
    }

    char c = scancode_to_ascii(scancode);
    if (!c)
        return;

    console_add_buffer(c);
}
