#include "arch/x86/io.h"
#include "arch/x86/pic.h"
#include "arch/x86/pit.h"
#include "arch/x86/time/time.h"
#include "drivers/console.h"
#include "drivers/keyboard.h"
#include "drivers/printk.h"
#include "sys/process.h"
#include "sys/timer.h"

#include <stdint.h>

extern tty_t tty;
extern int32_t ticks_remaining;
extern context_t* scheduler_context;
extern bool volatile need_resched;

uint64_t volatile ticks = 0;

__attribute__((target("general-regs-only"), interrupt)) void
timer_handler(registers_t* regs)
{
    (void)regs;
    pic_send_eoi(0);

    ticks += 1;
    check_timers();

    if (ticks % HZ == 0) {
        time_t new_epoch = getepoch() + 1;
        setepoch(new_epoch);

        proc_t* proc = myproc();
        if (proc && proc->state == RUNNING) {
            ticks_remaining -= 1;
            if (ticks_remaining <= 0) {
                need_resched = true;
                printk("Timer: scheduling needed for PID %d\n", proc->pid);
            }
        }
    }

    check_resched();
}

__attribute__((target("general-regs-only"), interrupt)) void
keyboard_handler(registers_t* regs)
{
    (void)regs;
    pic_send_eoi(1);

    int32_t scancode = inb(KEYBOARD_DATA_PORT);
    if ((tty.tail + 1) % 256 != tty.head) {
        tty.buf[tty.tail] = scancode;
        tty.tail = (tty.tail + 1) % 256;
    }

    check_resched();
}

__attribute__((target("general-regs-only"), interrupt)) void
spurious_handler(registers_t* regs)
{
    (void)regs;

    uint8_t isr = pic_get_isr();

    if (!(isr & 0x80)) {
        printk("Spurious IRQ 7 on Master PIC handled. No EOI sent.\n");
        return;
    }

    printk("Real IRQ 7 received. Sending EOI.\n");
    pic_send_eoi(0);
    check_resched();
}
