#ifndef KMALLOC_H
#define KMALLOC_H

#include <stddef.h>

size_t ksize(void *ptr);

void kfree(void *ptr);

void *kmalloc(size_t);

void kmalloc_init(void);

#endif /* KMALLOC_H */
