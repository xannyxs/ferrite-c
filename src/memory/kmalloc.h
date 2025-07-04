#ifndef KMALLOC_H
#define KMALLOC_H

#include <stddef.h>
#include <stdint.h>

#define HEAP_START (void *)0xD0000000

typedef struct block_header {
  size_t size;
  uint32_t magic;
} block_header_t;

void kmalloc_init();

void *kmalloc(size_t);

void kfree(void *ptr);

size_t ksize(void *ptr);

void kmem_init();

void *get_current_break();

void *kbrk(void *addr);

#endif /* KMALLOC_H */
