#ifndef BUDDY_H
#define BUDDY_H

#include <stddef.h>
#include <stdint.h>

#define MAX_ORDER 32
#define MIN_ORDER 0

typedef struct buddy_node {
  struct buddy_node *next;
} buddy_node_t;

typedef struct {
  uintptr_t base;
  size_t size;
  struct buddy_node *free_lists[MAX_ORDER + 1];
  uint8_t *map;
  size_t map_size;
  uint8_t max_order;
} buddy_allocator_t;

void buddy_init(void);

void *buddy_alloc(uint32_t order);

void buddy_dealloc(uintptr_t addr, uint32_t order);

#endif /* BUDDY_H */
