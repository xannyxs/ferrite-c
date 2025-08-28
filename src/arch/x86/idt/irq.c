#include "arch/x86/io.h"
#include "arch/x86/pic.h"
#include "arch/x86/pit.h"
#include "arch/x86/time/time.h"
#include "drivers/keyboard.h"
#include "sys/tasks.h"

volatile uint64_t ticks = 0;

__attribute__((target("general-regs-only"), interrupt)) void
timer_handler(registers_t *regs) {
  (void)regs;

  ticks += 1;
  if (ticks % HZ == 0) {
    time_t new_epoch = getepoch() + 1;
    setepoch(new_epoch);
  }

  pic_send_eoi(0);
}

__attribute__((target("general-regs-only"), interrupt)) void
keyboard_handler(registers_t *regs) {
  (void)regs;

  int32_t scancode = inb(KEYBOARD_DATA_PORT);
  setscancode(scancode);

  schedule_task(keyboard_input);

  pic_send_eoi(1);
}
