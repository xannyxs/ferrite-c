#ifndef BUDDY_H
#define BUDDY_H

#include <stddef.h>
#include <stdint.h>

#define MAX_ORDER 32 // The theoretical maximum order.
#define MIN_ORDER 0  // The smallest order

// A node in a free list, stored within the free block itself.
typedef struct buddy_node {
  struct buddy_node *next;
} buddy_node_t;

typedef struct {
  uintptr_t base; // Starting physical address of the managed memory.
  size_t size;    // Total size of the managed memory in bytes.
  buddy_node_t *
      free_lists[MAX_ORDER + 1]; // Array of free lists, indexed by block order.
  uint8_t *map;      // Bitmap to track block states for efficient merging.
  size_t map_size;   // Size of the state bitmap in bytes.
  uint8_t max_order; // Actual max order, calculated from the total memory size.
} buddy_allocator_t;

void buddy_init(void);

void *buddy_alloc(uint32_t order);

void buddy_dealloc(uintptr_t addr, uint32_t order);

size_t buddy_get_total_memory(void);

void buddy_visualize(void);

#endif /* BUDDY_H */
