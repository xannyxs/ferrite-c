#ifndef MEMBLOCK_H
#define MEMBLOCK_H

#include <stddef.h>

#define HEAP_START (void *)0xD0000000

void kfree(void *ptr);

size_t ksize(void *ptr);

void *get_current_break(void);

void *kbrk(void *addr);

void kmem_init(void *);

void memblock_init(void);

void *memblock(size_t);

void *get_next_free_addr(void);

void *get_heap_end_addr(void);

#endif /* MEMBLOCK_H */
