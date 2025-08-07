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
  uint8_t max_order;
} buddy_allocator_t;

void buddy_init(uintptr_t, size_t);

#endif /* BUDDY_H */
