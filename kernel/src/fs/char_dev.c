#include <drivers/block/device.h>
#include <drivers/chrdev.h>
#include <ferrite/errno.h>
#include <ferrite/types.h>
#include <fs/vfs.h>
#include <sys/file/file.h>

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
