#ifndef MEMBLOCK_H
#define MEMBLOCK_H

#include <stdbool.h>
#include <stddef.h>

void memblock_init(void);

void *memblock(size_t);

void *get_next_free_addr(void);

void *get_heap_end_addr(void);

void memblock_deactivate(void);
void memblock_activate(void);
bool memblock_is_active(void);

#endif /* MEMBLOCK_H */
