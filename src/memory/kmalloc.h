#ifndef KMALLOC_H
#define KMALLOC_H

#include <stddef.h>
#include <stdint.h>

static uint32_t next_free_page = 0x110000;

void *kmalloc(size_t);

#endif /* KMALLOC_H */
