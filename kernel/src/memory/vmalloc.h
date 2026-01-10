#ifndef VMALLOC_H
#define VMALLOC_H

#include "memory/memory.h"

size_t vsize(void* ptr);

void vfree(void* ptr);

void* vmalloc(size_t);

void vmalloc_init(void);

free_list_t* get_free_list_head(void);

void set_free_list_head(free_list_t*);

#endif /* VMALLOC_H */
