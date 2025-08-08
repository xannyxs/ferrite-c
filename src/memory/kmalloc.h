#ifndef KMALLOC_H
#define KMALLOC_H

#include <stddef.h>
#include <stdint.h>

#define MAXIMUM_SLAB_ALLOCATION 16384
#define MAGIC 0xDEADBEEF

typedef struct block_header {
  uint8_t order;
  uint8_t flags; // WIP, ignore for now
  uint32_t magic;
} block_header_t;

void *kmalloc(size_t);

#endif /* KMALLOC_H */
