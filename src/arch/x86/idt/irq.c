#include "arch/x86/idt/idt.h"
#include "arch/x86/pic.h"
#include "arch/x86/pit.h"
#include "arch/x86/time/time.h"
#include "drivers/console.h"
#include "drivers/keyboard.h"
#include "drivers/printk.h"
#include "sys/process.h"
#include "sys/timer.h"
#include "types.h"

#include <stdbool.h>

extern tty_t tty;
extern s32 ticks_remaining;
extern context_t* scheduler_context;
extern bool volatile need_resched;

unsigned long long volatile ticks = 0;

__attribute__((target("general-regs-only"))) void
timer_handler(registers_t* regs)
{
    (void)regs;

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
    pic_send_eoi(0);
}

__attribute__((target("general-regs-only"))) void
keyboard_handler(registers_t* regs)
{
    (void)regs;

    s32 scancode = inb(KEYBOARD_DATA_PORT);
    if ((tty.tail + 1) % 256 != tty.head) {
        tty.buf[tty.tail] = scancode;
        tty.tail = (tty.tail + 1) % 256;
    }
    pic_send_eoi(1);
}

__attribute__((target("general-regs-only"))) void
spurious_handler(registers_t* regs)
{
    (void)regs;

    u8 isr = pic_get_isr();

    if (!(isr & 0x80)) {
        printk("Spurious IRQ 7 on Master PIC handled. No EOI sent.\n");
        return;
    }

    printk("Real IRQ 7 received. Sending EOI.\n");
    pic_send_eoi(0);
}

__attribute__((target("general-regs-only"))) void
irq_dispatcher_c(registers_t* regs)
{
    u32 irq_num = regs->int_no - 0x20;

    switch (regs->int_no) {
    case 0x20:
        timer_handler(regs);
        break;
    case 0x21:
        keyboard_handler(regs);
        break;
    case 0x27:
        spurious_handler(regs);
        break;
    default:
        printk("Unhandled IRQ: %d\n", irq_num);
        break;
    }

    check_resched();
}
