#include "memory/kmalloc.h"
#include "arch/x86/memlayout.h"
#include "lib/math.h"
#include "memory/buddy_allocator/buddy.h"
#include "memory/consts.h"
#include "memory/memory.h"

#include <stddef.h>
#include <stdint.h>

/* Public */

/**
 * @brief Allocates a physically contiguous memory block.
 *
 * Fast and suitable for most kernel allocations, especially for hardware/DMA
 * buffers that require physical contiguity.
 *
 * @param size The number of bytes to allocate.
 * @return A pointer to the allocated memory, or NULL on failure.
 */
void *kmalloc(size_t n) {
  if (n == 0 || n >= MAXIMUM_SLAB_ALLOCATION) {
    return NULL;
  }

  size_t total_size = ALIGN(n + sizeof(block_header_t), PAGE_SIZE);

  uint32_t num_pages = CEIL_DIV(total_size, PAGE_SIZE);
  uint32_t order = log2_rounded_up(num_pages);
  void *paddr = buddy_alloc(order);
  if (!paddr) {
    return NULL;
  }

  uintptr_t vaddr = P2V_WO((uintptr_t)paddr);
  block_header_t *header = (block_header_t *)vaddr;
  header->magic = MAGIC;
  header->size = total_size;
  header->flags = MEM_TYPE_KMALLOC;

  return (void *)(header + 1);
}
