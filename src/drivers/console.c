#include "drivers/console.h"
#include "arch/x86/cpu.h"
#include "arch/x86/entry.h"
#include "arch/x86/idt/syscalls.h"
#include "arch/x86/time/rtc.h"
#include "arch/x86/time/time.h"
#include "drivers/block/device.h"
#include "drivers/block/ide.h"
#include "drivers/printk.h"
#include "drivers/video/vga.h"
#include "ferrite/dirent.h"
#include "fs/stat.h"
#include "memory/buddy_allocator/buddy.h"
#include "memory/kmalloc.h"
#include "sys/file/fcntl.h"
#include "sys/process/process.h"
#include "sys/signal/signal.h"
#include "sys/timer/timer.h"

#include <ferrite/string.h>
#include <ferrite/types.h>
#include <lib/stdlib.h>

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
    printk("  rmdir   - Remove a existing directory\n");
    printk("  touch   - Create a new file\n");
    printk("  rm      - Remove a file\n");
    printk("  cat     - Concatenate files and print\n");
    printk("  echo    - Display a line of text\n");
    printk("  cd      - Change Directory\n");
    printk("  pwd     - Print Working Directory\n");
    printk("  mount   - Mount a device\n");
    printk("  umount  - Unmount a device\n");
    printk("  insmod  - Load a module\n");
    printk("  help    - Show this help message\n");
}
static void echo_file(char const* args)
{
    char* redirect = strchr(args, '>');
    if (!redirect) {
        printk("%s\n", args);
        return;
    }

    size_t text_len = redirect - args;
    while (text_len > 0 && args[text_len - 1] == ' ') {
        text_len--;
    }

    char text[256];
    memcpy(text, args, text_len);
    text[text_len] = '\0';

    char* filename = redirect + 1;
    while (*filename == ' ') {
        filename++;
    }

    if (*filename == '\0') {
        printk("echo: missing filename\n");
        return;
    }

    int fd;
    __asm__ volatile("int $0x80"
                     : "=a"(fd)
                     : "a"(SYS_OPEN), "b"(filename), "c"(O_CREAT | O_WRONLY),
                       "d"(0644)
                     : "memory");

    if (fd < 0) {
        printk("echo (%d): cannot write to %s\n", fd, filename);
        return;
    }

    int ret;

    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_WRITE), "b"(fd), "c"(text), "d"(text_len)
                     : "memory");

    __asm__ volatile("int $0x80" : : "a"(SYS_CLOSE), "b"(fd) : "memory");
}

static void mount(char const* arg)
{
    char** vars = split(arg, ' ');
    if (!vars) {
        return;
    }

    if (!vars[0] || !vars[1]) {
        goto cleanup;
    }

    int ret = 0;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_MOUNT), "b"(vars[0]), "c"(vars[1]), "d"("ext2"),
                       "S"(0), "D"(NULL)
                     : "memory");

    if (ret < 0) {
        printk("mount: failed with error %d\n", ret);
    } else {
        printk("mount: successfully mounted %s on %s\n", vars[0], vars[1]);
    }

cleanup:
    for (int i = 0; vars[i]; i += 1) {
        kfree(vars[i]);
    }

    kfree(vars);
}

static void umount(char const* device)
{
    int ret = 0;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_UMOUNT), "b"(device), "c"(0)
                     : "memory");
}

static void insmod(char const* path)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_INIT_MODULE), "b"(path), "c"(0), "d"(NULL)
                     : "memory");

    printk("ret: %d\n", ret);
}

static void cat_file(char const* path)
{
    int fd;
    __asm__ volatile("int $0x80"
                     : "=a"(fd)
                     : "a"(SYS_OPEN), "b"(path), "c"(O_RDONLY), "d"(0)
                     : "memory");

    if (fd < 0) {
        printk("cat: %s: No such file or directory\n", path);
        return;
    }

    char buf[512];
    int bytes_read;

    while (1) {
        __asm__ volatile("int $0x80"
                         : "=a"(bytes_read)
                         : "a"(SYS_READ), "b"(fd), "c"(buf), "d"(sizeof(buf))
                         : "memory");

        if (bytes_read <= 0) {
            break;
        }

        for (int i = 0; i < bytes_read; i++) {
            printk("%c", buf[i]);
        }
    }

    __asm__ volatile("int $0x80" : : "a"(SYS_CLOSE), "b"(fd) : "memory");
}

static void change_directory(char const* path)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_CHDIR), "b"(path)
                     : "memory");
}

static void print_working_directory(void)
{
    int ret;
    u8 buf[512];
    long size = 512;

    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_GETCWD), "b"(buf), "c"(size)
                     : "memory");
    if (ret < 0) {
        printk("Failed to show pwd: error code: %d\n", ret);
    } else {
        printk("%s\n", buf);
    }
}

static void touch_file(char const* path)
{
    int fd;
    __asm__ volatile("int $0x80"
                     : "=a"(fd)
                     : "a"(SYS_OPEN), "b"(path), "c"(O_CREAT), "d"(0644)
                     : "memory");

    if (fd < 0) {
        printk("Failed to create file: %s with error code: %d\n", path, fd);
        return;
    }

    __asm__ volatile("int $0x80" : : "a"(SYS_CLOSE), "b"(fd) : "memory");
}

static void remove_file(char const* path)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_UNLINK), "b"(path)
                     : "memory");

    if (ret < 0) {
        printk("Failed to unlink file: %s with error code: %d\n", path, ret);
        return;
    }

    __asm__ volatile("int $0x80" : : "a"(SYS_CLOSE), "b"(ret) : "memory");
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

static void remove_directory(char const* path)
{
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(40), "b"(path) : "memory");

    if (ret < 0) {
        printk(
            "Something went wrong removing directory with error code %d\n", ret
        );
    }
}

static void list_directory_contents(char const* path)
{
    int ret = 1;
    u8 buf[512];
    long size = 512;

    if (!path) {
        __asm__ volatile("int $0x80"
                         : "=a"(ret)
                         : "a"(SYS_GETCWD), "b"(buf), "c"(size)
                         : "memory");
    }

    char const* dir = path ? path : (char*)buf;

    // Open File
    int fd;
    __asm__ volatile("int $0x80"
                     : "=a"(fd)
                     : "a"(5), "b"(dir), "c"(0), "d"(0)
                     : "memory");
    if (fd < 0) {
        printk("Could not open dir\n");
        return;
    }

    dirent_t dirent = { 0 };

    while (1) {
        __asm__ volatile("int $0x80"
                         : "=a"(ret)
                         : "a"(SYS_READDIR), "b"(fd), "c"(&dirent), "d"(1)
                         : "memory");
        if (ret <= 0) {
            break;
        }

        char fullpath[256];
        strlcpy(fullpath, dir, sizeof(fullpath));

        size_t len = strlen(fullpath);
        if (len > 0 && fullpath[len - 1] != '/') {
            fullpath[len] = '/';
            fullpath[len + 1] = '\0';
        }

        strlcat(fullpath, (char*)dirent.d_name, sizeof(fullpath));

        struct stat st;
        int stat_ret;
        __asm__ volatile("int $0x80"
                         : "=a"(stat_ret)
                         : "a"(18), "b"(fullpath), "c"(&st)
                         : "memory");

        if (stat_ret == 0) {
            printk("%c", S_ISDIR(st.st_mode) ? 'd' : '-');
            printk("%c", (st.st_mode & S_IRUSR) ? 'r' : '-');
            printk("%c", (st.st_mode & S_IWUSR) ? 'w' : '-');
            printk("%c", (st.st_mode & S_IXUSR) ? 'x' : '-');
            printk("%c", (st.st_mode & S_IRGRP) ? 'r' : '-');
            printk("%c", (st.st_mode & S_IWGRP) ? 'w' : '-');
            printk("%c", (st.st_mode & S_IXGRP) ? 'x' : '-');
            printk("%c", (st.st_mode & S_IROTH) ? 'r' : '-');
            printk("%c", (st.st_mode & S_IWOTH) ? 'w' : '-');
            printk("%c", (st.st_mode & S_IXOTH) ? 'x' : '-');
            printk(
                " %2d %8d %10u %s\n", st.st_nlink, st.st_size, st.st_mtime,
                (char*)dirent.d_name
            );
        } else {
            printk("?????????? ?? ???????? %s\n", (char*)dirent.d_name);
        }
    }

    if (ret < 0) {
        printk("Something went wrong reading dir\n");
    }

    __asm__ volatile("int $0x80" : "=a"(fd) : "a"(6), "b"(fd) : "memory");
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

static char* get_arg(void)
{
    char* arg = strchr(buffer, ' ');
    return arg ? arg + 1 : NULL;
}

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
                                            { "pwd", print_working_directory },
                                            { NULL, NULL } };

    printk("\n");

    for (s32 cmd = 0; command_table[cmd].cmd; cmd++) {
        if (strcmp(buffer, command_table[cmd].cmd) == 0) {
            command_table[cmd].f();
            goto cleanup;
        }
    }

    char* arg = get_arg();

    if (strncmp(buffer, "ls", 2) == 0
        && (buffer[2] == ' ' || buffer[2] == '\0')) {
        list_directory_contents(arg);
    } else if (strncmp(buffer, "mkdir", 5) == 0 && buffer[5] == ' ') {
        if (arg) {
            make_directory(arg);
        }
    } else if (strncmp(buffer, "touch", 5) == 0 && buffer[5] == ' ') {
        if (arg) {
            touch_file(arg);
        }
    } else if (strncmp(buffer, "rmdir", 5) == 0 && buffer[5] == ' ') {
        if (arg) {
            remove_directory(arg);
        }
    } else if (strncmp(buffer, "rm", 2) == 0 && buffer[2] == ' ') {
        if (arg) {
            remove_file(arg);
        }
    } else if (strncmp(buffer, "cat", 3) == 0 && buffer[3] == ' ') {
        if (arg) {
            cat_file(arg);
        }
    } else if (strncmp(buffer, "echo", 4) == 0 && buffer[4] == ' ') {
        if (arg) {
            echo_file(arg);
        }
    } else if (strncmp(buffer, "cd", 2) == 0 && buffer[2] == ' ') {
        if (arg) {
            change_directory(arg);
        }
    } else if (strncmp(buffer, "mount", 5) == 0 && buffer[5] == ' ') {
        if (arg) {
            mount(arg);
        }
    } else if (strncmp(buffer, "umount", 6) == 0 && buffer[6] == ' ') {
        if (arg) {
            umount(arg);
        }
    } else if (strncmp(buffer, "insmod", 6) == 0 && buffer[6] == ' ') {
        if (arg) {
            insmod(arg);
        }
    }

cleanup:
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
