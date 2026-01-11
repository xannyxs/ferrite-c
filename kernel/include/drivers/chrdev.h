#ifndef CHRDEV_H
#define CHRDEV_H

#include <ferrite/types.h>

typedef struct {
    int (*read)(dev_t dev, void* buf, size_t count, off_t* offset);
    int (*write)(dev_t dev, void const* buf, size_t count, off_t* offset);
    int (*open)(dev_t dev);
    int (*close)(dev_t dev);
} chrdev_ops_t;

int register_chrdev(unsigned int major, chrdev_ops_t const* ops);

chrdev_ops_t const* get_chrdev(unsigned int major);

int unregister_chrdev(unsigned int major);

#endif /* CHRDEV_H */
