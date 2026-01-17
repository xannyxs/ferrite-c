#include "cpu.h"
#include <drivers/chrdev.h>
#include <drivers/printk.h>
#include <drivers/vga.h>
#include <ferrite/string.h>
#include <ferrite/types.h>
#include <sys/file/file.h>
#include <sys/process/process.h>
#include <sys/signal/signal.h>

#define PROMPT "[42]$ "

typedef struct {
    u8 buffer[256];
    size_t head;
    size_t tail;
} ScanBuffer;

static ScanBuffer SCAN_BUFFER = { .buffer = { 0 }, .head = 0, .tail = 0 };

void tty_write(u8 scancode)
{
    if ((SCAN_BUFFER.tail + 1) % 256 != SCAN_BUFFER.head) {
        SCAN_BUFFER.buffer[SCAN_BUFFER.tail] = scancode;
        SCAN_BUFFER.tail = (SCAN_BUFFER.tail + 1) % 256;
    }
}

u8 tty_read(void)
{
    if (SCAN_BUFFER.head == SCAN_BUFFER.tail) {
        return 0;
    }
    u8 byte = SCAN_BUFFER.buffer[SCAN_BUFFER.head];
    SCAN_BUFFER.head = (SCAN_BUFFER.head + 1) % 256;
    return byte;
}

int tty_is_empty(void) { return SCAN_BUFFER.head == SCAN_BUFFER.tail; }

typedef struct {
    u8 cmd_buffer[256];
    size_t cmd_len;
} Tty;

static void tty_add_buffer(Tty* self, u8 c)
{
    // Ctrl+C
    if (c == '\x03') {
        vga_puts("^C\n");
        do_kill(myproc()->pid, SIGINT);
        self->cmd_len = 0;

        memset(self->cmd_buffer, 0, 256);
        vga_puts(PROMPT);
    }
}

static Tty TTY = { .cmd_buffer = { 0 }, .cmd_len = 0 };

void console_add_buffer(u8 ch) { tty_add_buffer(&TTY, ch); }

static int
console_dev_read(vfs_inode_t* inode, file_t* file, void* buf_ptr, int count)
{
    (void)inode;
    (void)file;

    u8* buf = (u8*)buf_ptr;
    size_t read_count = 0;

    while (read_count < (size_t)count) {
        while (tty_is_empty()) {
            sti();
            halt();
        }

        u8 ch = tty_read();
        if (ch == 0) {
            break;
        }
        buf[read_count] = ch;
        read_count++;
        if (ch == '\n') {
            break;
        }
    }

    return (int)read_count;
}

static int console_dev_write(
    vfs_inode_t* inode,
    file_t* file,
    void const* buf_ptr,
    int count
)
{
    (void)inode;
    (void)file;

    u8 const* buf = (u8 const*)buf_ptr;
    for (size_t i = 0; i < (size_t)count; i++) {
        vga_putchar(buf[i]);
    }

    return count;
}

static const struct file_operations console_ops = { .readdir = NULL,
                                                    .read = console_dev_read,
                                                    .write = console_dev_write,
                                                    .open = NULL,
                                                    .release = NULL,
                                                    .lseek = NULL };

void console_chrdev_init(void) { register_chrdev(5, &console_ops); }
