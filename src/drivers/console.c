#include "drivers/console.h"
#include "arch/x86/cpu.h"
#include "arch/x86/entry.h"
#include "arch/x86/time/rtc.h"
#include "arch/x86/time/time.h"
#include "drivers/block/device.h"
#include "drivers/block/ide.h"
#include "drivers/printk.h"
#include "drivers/video/vga.h"
#include "ferrite/dirent.h"
#include "memory/buddy_allocator/buddy.h"
#include "sys/file/stat.h"
#include "sys/process/process.h"
#include "sys/signal/signal.h"
#include "sys/timer/timer.h"

#include <ferrite/types.h>
#include <lib/stdlib.h>
#include <lib/string.h>

extern proc_t* current_proc;

static char const* prompt = "[42]$ ";
static char buffer[VGA_WIDTH];
static s32 i = 0;

tty_t tty;

/* Private */

static void delete_character(void)
{
    if (i == 0) {
        return;
    }

    buffer[i] = 0;
    i -= 1;
    vga_clear_char();
}

static void print_help(void)
{
    printk("Available commands:\n");
    printk("  reboot  - Restart the system\n");
    printk("  gdt     - Print Global Descriptor Table\n");
    printk("  idt     - Print Interrupt Descriptor Table\n");
    printk("  clear   - Clear the screen\n");
    printk("  time    - See the current time\n");
    printk("  epoch   - See the current time since Epoch\n");
    printk("  memory  - Show the current memory allocation of the buddy "
           "allocator\n");
    printk("  top     - Show all active processes\n");
    printk("  devices - Show all found devices\n");
    printk("  ls      - List information about the FILEs\n");
    printk("  mkdir   - Create a new directory\n");
    printk("  help    - Show this help message\n");
}

static void print_devices(void)
{
    block_device_t* d = get_devices();

    int found = 0;
    for (int i = 0; i < MAX_BLOCK_DEVICES; i += 1) {
        if (!d[i].d_data) {
            continue;
        }

        found++;
        printk("[%d] ", i);

        switch (d[i].d_type) {
        case BLOCK_DEVICE_IDE:
            printk("IDE   ");
            break;
        case BLOCK_DEVICE_SATA:
            printk("SATA  ");
            break;
        case BLOCK_DEVICE_NVME:
            printk("NVME  ");
            break;
        default:
            printk("UNKNOWN ");
            break;
        }

        if (d[i].d_type == BLOCK_DEVICE_IDE) {
            ata_drive_t* drive = (ata_drive_t*)d[i].d_data;
            printk("| %s | ", drive->drive ? "Slave " : "Master ");
            printk("%u sectors ", drive->lba28_sectors);

            u32 size_mb = (drive->lba28_sectors * 512) / (1024 * 1024);
            printk("(%u MB) ", size_mb);

            if (drive->supports_lba48) {
                printk("| LBA48 ");
            }

            if (drive->name[0]) {
                printk("| Model: %s", drive->name);
            }
        }

        printk("\n");
    }

    if (found == 0) {
        printk("No block devices found.\n");
    } else {
        printk("Total: %d device(s)\n", found);
    }
}

static void print_idt(void)
{
    descriptor_pointer_t idtr;

    __asm__ volatile("sidt %0" : "=m"(idtr));

    printk("IDT Base Address: 0x%x\n", idtr.base);
    printk("IDT Limit: 0x%xx\n", idtr.limit);
}

static void make_directory(char const* path)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(39), "b"(path), "c"(0755)
                     : "memory");

    if (ret < 0) {
        printk("Something went wrong making directory\n");
    }
}

static void list_directory_contents(char const* path)
{
    // Open File
    int fd;
    __asm__ volatile("int $0x80"
                     : "=a"(fd)
                     : "a"(5), "b"(path ? path : "/"), "c"(0), "d"(0)
                     : "memory");
    if (fd < 0) {
        printk("Could not open dir\n");
        return;
    }

    int ret = 1;
    dirent_t dirent = { 0 };

    while (1) {
        __asm__ volatile("int $0x80"
                         : "=a"(ret)
                         : "a"(89), "b"(fd), "c"(&dirent), "d"(1)
                         : "memory");
        if (ret == 0) {
            break;
        }

        // ls -l style output with placeholders
        printk("?????????? ?? ???????? %s\n", (char*)dirent.d_name);
    }

    if (ret < 0) {
        printk("Something went wrong reading dir\n");
    }

    // TODO: Close fd when syscall 6 is implemented
}

static void print_time(void)
{
    rtc_time_t t;
    gettime(&t);

    // print the time in a standard format (e.g., hh:mm:ss dd/mm/yy)
    printk(
        "current time: %u%u:%u%u:%u%u %u%u/%u%u/%u\n", t.hour / 10, t.hour % 10,
        t.minute / 10, t.minute % 10, t.second / 10, t.second % 10, t.day / 10,
        t.day % 10, t.month / 10, t.month % 10, t.year
    );
}

static void print_epoch(void)
{
    time_t t = getepoch();

    printk("current Epoch Time: %u\n", t);
}

static void print_gdt(void)
{
    descriptor_pointer_t gdtr;

    __asm__ volatile("sgdt %0" : "=m"(gdtr));

    printk("GDT Base Address: 0x%x\n", gdtr.base);
    printk("GDT Limit: 0x%x\n", gdtr.limit);
}

static void print_buddy(void) { buddy_visualize(); }

static void exec_sleep(void) { ksleep(3); }

static void exec_abort(void) { abort("Test abort"); }

static void execute_buffer(void)
{
    static exec_t const command_table[] = { { "reboot", reboot },
                                            { "gdt", print_gdt },
                                            { "memory", print_buddy },
                                            { "clear", vga_init },
                                            { "help", print_help },
                                            { "panic", exec_abort },
                                            { "idt", print_idt },
                                            { "time", print_time },
                                            { "epoch", print_epoch },
                                            { "top", process_list },
                                            { "devices", print_devices },
                                            { "sleep", exec_sleep },
                                            { NULL, NULL } };

    printk("\n");

    for (s32 cmd = 0; command_table[cmd].cmd; cmd++) {
        if (command_table[cmd].f
            && strcmp(buffer, command_table[cmd].cmd) == 0) {
            command_table[cmd].f();
            break;
        }
    }

    if (strncmp("ls", buffer, 2) == 0) {
        char* tmp = strchr(buffer, ' ');
        if (tmp) {
            tmp++;
        }

        list_directory_contents(tmp);
    }

    if (strncmp("mkdir", buffer, 5) == 0) {
        char* tmp = strchr(buffer, ' ');
        if (tmp) {
            tmp++;
        }

        make_directory(tmp);
    }

    memset(buffer, 0, VGA_WIDTH);
    i = 0;

    printk("%s", prompt);
}

/* Public */

void console_init(void)
{
    tty.head = 0;
    tty.tail = 0;
    tty.shell_pid = myproc()->pid;
    memset(tty.buf, 0, 256);

    printk("%s", prompt);
}

void console_add_buffer(char c)
{
    switch (c) {
    case '\x03':
        printk("^C\n");
        do_kill(current_proc->pid, SIGINT);
        break;
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
