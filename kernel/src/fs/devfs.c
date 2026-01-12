#include "idt/syscalls.h"
#include <drivers/block/device.h>
#include <ferrite/errno.h>
#include <ferrite/string.h>
#include <ferrite/types.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <lib/stdlib.h>
#include <memory/kmalloc.h>
#include <sys/file/fcntl.h>

#include <drivers/printk.h>

extern struct inode_operations chrdev_inode_ops;
extern void console_chrdev_init(void);

static inline int k_mkdir(char const* path, int mode)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(SYS_MKDIR), "b"(path), "c"(mode)
                     : "memory");
    return ret;
}

void devfs_init(void)
{
    console_chrdev_init();

    int ret = k_mkdir("/dev", 0755);
    if (ret < 0 && ret != -EEXIST) {
        abort("Something went wrong while creating /dev");
    }

    vfs_mknod("/dev/console", S_IFCHR | 0666, MKDEV(5, 1));
}
