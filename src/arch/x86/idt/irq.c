#include "arch/x86/io.h"
#include "arch/x86/pic.h"
#include "arch/x86/pit.h"
#include "arch/x86/time/time.h"
#include "drivers/keyboard.h"
#include "drivers/printk.h"
#include "sys/process.h"
#include "sys/tasks.h"

#include <stdint.h>

extern int32_t ticks_remaining;
extern context_t *scheduler_context;
volatile uint64_t ticks = 0;

__attribute__((target("general-regs-only"), interrupt)) void
timer_handler(registers_t *regs) {
  (void)regs;

  pic_send_eoi(0);

  ticks += 1;
  if (ticks % HZ == 0) {
    time_t new_epoch = getepoch() + 1;
    setepoch(new_epoch);

    proc_t *current_proc = myproc();
    printk("Tick remaining %d in PID %d\n", ticks_remaining, current_proc->pid);
    if (current_proc && current_proc->state == RUNNING) {
      ticks_remaining -= 1;
      if (ticks_remaining <= 0) {
        current_proc->state = READY;
        swtch(&current_proc->context, scheduler_context);
      }
    }
  }
}

__attribute__((target("general-regs-only"), interrupt)) void
keyboard_handler(registers_t *regs) {
  (void)regs;

  int32_t scancode = inb(KEYBOARD_DATA_PORT);
  setscancode(scancode);

  schedule_task(keyboard_input);

  pic_send_eoi(1);
}

#include "drivers/printk.h"

__attribute__((target("general-regs-only"), interrupt)) void
spurious_handler(registers_t *regs) {
  (void)regs;

  uint8_t isr = pic_get_isr();

  if (!(isr & 0x80)) {
    printk("Spurious IRQ 7 on Master PIC handled. No EOI sent.\n");
    return;
  }

  printk("Real IRQ 7 received. Sending EOI.\n");
  pic_send_eoi(0);
}
