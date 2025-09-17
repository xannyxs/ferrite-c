#include "drivers/console.h"
#include "drivers/keyboard.h"
#include "drivers/printk.h"

#include <stdbool.h>

extern tty_t tty;
extern proc_t ptables[NUM_PROC];

void process_list(void) {
  printk("PID  STATE  NAME\n");
  printk("---  -----  ----\n");

  for (int32_t i = 0; i < NUM_PROC; i += 1) {
    if (ptables[i].state != UNUSED) {
      const char *state_str[] = {"UNUSED", "EMBRYO",  "SLEEPING",
                                 "READY",  "RUNNING", "ZOMBIE"};

      printk("%d  %s  %s\n", ptables[i].pid, state_str[ptables[i].state],
             ptables[i].name);
    }
  }
}

void shell_process(void) {
  console_init();

  while (true) {
    while (tty.tail != tty.head) {
      uint8_t scancode = tty.buf[tty.head];
      tty.head = (tty.head + 1) % 256;

      keyboard_put(scancode);
    }

    yield();
  }
}
