#include "memory/kmalloc.h"
#include "memory/buddy_allocator/buddy.h"
#include "memory/consts.h"
#include "memory/memblock.h"
#include "memory/vmm.h"

#include <stddef.h>
#include <stdint.h>

/* Public */

static uintptr_t next_free_vaddr = (uintptr_t)HEAP_START;

void *kmalloc(size_t num_bytes) {
  if (num_bytes == 0) {
    return NULL;
  }

  size_t total_size_needed = num_bytes + sizeof(block_header_t);
  int order = 0;
  while (((uint32_t)PAGE_SIZE * (1 << order)) < total_size_needed) {
    order++;
  }

  uintptr_t phys_addr = (uintptr_t)buddy_alloc(order);
  if (phys_addr == 0) {
    return NULL;
  }

  uintptr_t virt_addr = next_free_vaddr;
  size_t allocation_size = PAGE_SIZE * (1 << order);
  for (size_t i = 0; i < allocation_size; i += PAGE_SIZE) {
    vmm_map_page((void *)(phys_addr + i), (void *)(virt_addr + i), 0);
  }

  next_free_vaddr += allocation_size;

  block_header_t *header = (block_header_t *)virt_addr;
  header->magic = MAGIC;
  header->order = order;

  return (void *)(header + 1);
}
