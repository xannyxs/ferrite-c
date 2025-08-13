#ifndef KMALLOC_H
#define KMALLOC_H

#include <stddef.h>
#include <stdint.h>

#define HEAP_START (void *)0xD0000000
#define MAXIMUM_SLAB_ALLOCATION 16384
#define MAGIC 0xDEADBEEF

typedef struct free_list {
  size_t size;
  struct free_list *next;
} free_list_t;

typedef struct block_header {
  uint8_t flags; // WIP, ignore for now
  size_t size;
  uint32_t magic;
} block_header_t;

void kfree(void *ptr);

void *kbrk(void *addr);

void *kmalloc(size_t);

void kmalloc_init(void);

free_list_t *get_free_list_head(void);

void set_free_list_head(free_list_t *);

#endif /* KMALLOC_H */
