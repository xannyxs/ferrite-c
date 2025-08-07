#ifndef KMALLOC_H
#define KMALLOC_H

#include <stddef.h>

enum alloc_e { NONE, MEMBLOCK, BUDDY, SLAB };

enum alloc_e get_allocator(void);

void set_allocator(enum alloc_e new_alloc);

void *kmalloc(size_t);

#endif /* KMALLOC_H */
