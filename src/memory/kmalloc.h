#ifndef KMALLOC_H
#define KMALLOC_H

#include <stddef.h>

void kfree(void *ptr);

void *kmalloc(size_t);

void kmalloc_init(void);

#endif /* KMALLOC_H */
