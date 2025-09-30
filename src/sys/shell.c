#include "drivers/console.h"
#include "drivers/keyboard.h"
#include "drivers/printk.h"
#include "sys/process.h"

#include <stdbool.h>

extern tty_t tty;
extern proc_t ptables[NUM_PROC];

void process_list(void)
{
    printk("PID  STATE     NAME              PPID  PARENT ADDRESS\n");
    printk("---  --------  ----------------  ----  --------------\n");
    for (int32_t i = 0; i < NUM_PROC; i += 1) {
        if (ptables[i].state != UNUSED) {
            char const* state_str[] = { "UNUSED", "EMBRYO", "SLEEPING",
                "READY", "RUNNING", "ZOMBIE" };
            pid_t ppid = ptables[i].parent ? ptables[i].parent->pid : 0;
            printk("%3d  %8s  %16s  %4d  0x%08x\n", ptables[i].pid,
                state_str[ptables[i].state], ptables[i].name, ppid,
                ptables[i].parent);
        }
    }
}

void shell_process(void)
{
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
