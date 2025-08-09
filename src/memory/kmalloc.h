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

void kmem_init(void *);

void *get_current_break(void);

void kfree(void *ptr);

size_t ksize(void *ptr);

void *kbrk(void *addr);

void *kmalloc(size_t);

#endif /* KMALLOC_H */
