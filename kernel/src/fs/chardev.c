#include "idt/syscalls.h"
#include <drivers/block/device.h>
#include <drivers/chrdev.h>
#include <ferrite/errno.h>
#include <ferrite/string.h>
#include <ferrite/types.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <lib/stdlib.h>
#include <memory/kmalloc.h>
#include <sys/file/fcntl.h>
#include <sys/file/file.h>

extern void console_chrdev_init(void);

static int chrdev_open(vfs_inode_t* node, file_t* file)
{
    int i = MAJOR(node->i_rdev);
    const struct file_operations* ops = get_chrdev(i);
    if (!ops) {
        return -ENODEV;
    }
    file->f_op = (struct file_operations*)ops;
    if (file->f_op->open) {
        return file->f_op->open(node, file);
    }
    return 0;
}

struct file_operations chrdev_file_ops = {
    .read = NULL,
    .write = NULL,
    .readdir = NULL,
    .open = chrdev_open,
    .release = NULL,
    .lseek = NULL,
};

struct inode_operations chrdev_inode_ops
    = { .default_file_ops = &chrdev_file_ops,
        .create = NULL,
        .lookup = NULL,
        .unlink = NULL,
        .mkdir = NULL,
        .rmdir = NULL,
        .truncate = NULL,
        .permission = NULL };

void devfs_init(void)
{
    console_chrdev_init();

    int ret = sys_mkdir("/dev", 0755);
    if (ret < 0 && ret != -EEXIST) {
        abort("Failed to create /dev directory");
    }

    vfs_mknod("/dev/console", S_IFCHR | 0666, MKDEV(5, 1));
}
