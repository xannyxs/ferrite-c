#ifndef MEMBLOCK_H
#define MEMBLOCK_H

#include <stddef.h>

void memblock_init(void);

void *memblock(size_t);

void *get_next_free_addr(void);

void *get_heap_end_addr(void);

#endif /* MEMBLOCK_H */
