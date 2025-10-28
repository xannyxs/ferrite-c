#ifndef BUDDY_H
#define BUDDY_H

#include "types.h"

#define MAX_ORDER 32
#define MIN_ORDER 0

// A node in a free list, stored within the free block itself.
typedef struct buddy_node {
    struct buddy_node* next;
} buddy_node_t;

typedef struct {
    vaddr_t base; // Starting physical address of the managed memory.
    size_t size;  // Total size of the managed memory in bytes.
    buddy_node_t* free_lists[MAX_ORDER + 1]; // Array of free lists, indexed by
                                             // block order.
    u8* map;         // Bitmap to track block states for efficient merging.
    size_t map_size; // Size of the state bitmap in bytes.
    u8 max_order;    // Actual max order, calculated from the total memory size.
} buddy_allocator_t;

void buddy_init(void);

void* buddy_alloc(u32 order);

void buddy_dealloc(paddr_t paddr, u32 order);

u32 buddy_get_max_order(void);

size_t buddy_get_total_memory(void);

void buddy_visualize(void);

#endif /* BUDDY_H */
