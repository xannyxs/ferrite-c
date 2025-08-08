#include "memory/kmalloc.h"
#include "drivers/printk.h"
#include "memory/buddy_allocator/buddy.h"
#include "memory/consts.h"

#include <stddef.h>
#include <stdint.h>

/* Public */

void *kmalloc(size_t num_bytes) {
  if (num_bytes == 0) {
    return NULL;
  }

  uint32_t total_size_needed = num_bytes + sizeof(block_header_t);
  uint32_t order = 0;
  while (((uint32_t)PAGE_SIZE * (1 << order)) < total_size_needed) {
    order += 1;
  }

  void *ptr = buddy_alloc(total_size_needed);
  if (!ptr) {
    return NULL;
  }

  block_header_t *header = (block_header_t *)ptr;
  header->magic = MAGIC;
  header->order = order;

  return (void *)(header + 1);
}
