#ifndef MEMBLOCK_H
#define MEMBLOCK_H

#include <stddef.h>
#include <stdint.h>

#define HEAP_START (void *)0xD0000000
#define MAGIC 0xDEADBEEF

typedef struct block_header {
  size_t size;
  uint32_t magic;
} block_header_t;

void memblock_init(void);

void *memblock(size_t);

void kfree(void *ptr);

size_t ksize(void *ptr);

void kmem_init(void);

void *get_current_break(void);

void *kbrk(void *addr);

#endif /* MEMBLOCK_H */
