#ifndef CHRDEV_H
#define CHRDEV_H

#include <ferrite/types.h>
#include <sys/file/file.h>

struct file_operations const* get_chrdev(unsigned int major);

int register_chrdev(unsigned int major, struct file_operations const* ops);

int unregister_chrdev(unsigned int major);

#endif /* CHRDEV_H */
