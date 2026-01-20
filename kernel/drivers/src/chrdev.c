#include "sys/file/file.h"

#include <drivers/chrdev.h>
#include <uapi/errno.h>
#include <types.h>

#define MAX_CHRDEV 32

const struct file_operations* chrdev_table[MAX_CHRDEV] = { 0 };

int register_chrdev(unsigned int major, const struct file_operations* ops)
{
    if (major >= MAX_CHRDEV) {
        return -EINVAL;
    }

    if (chrdev_table[major] != NULL) {
        return -EBUSY;
    }

    chrdev_table[major] = ops;
    return 0;
}

struct file_operations const* get_chrdev(unsigned int major)
{
    if (major >= MAX_CHRDEV) {
        return NULL;
    }

    return chrdev_table[major];
}

int unregister_chrdev(unsigned int major)
{
    if (major >= MAX_CHRDEV) {
        return -EINVAL;
    }

    chrdev_table[major] = NULL;
    return 0;
}
