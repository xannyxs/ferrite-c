#include "memory/memblock.h"
#include "memory/kmalloc.h"

#include <stddef.h>
#include <stdint.h>

static void *heap_start_addr = NULL;
static void *next_free_addr = NULL;

void memblock_init(void) {
  kmem_init();
  heap_start_addr = HEAP_START;
  next_free_addr = heap_start_addr;

  set_allocator(MEMBLOCK);
}

void *memblock(size_t num_bytes) {
  if (num_bytes == 0) {
    return NULL;
  }

  void *heap_end_addr = get_current_break();
  void *total_needed_space =
      (void *)((uint32_t)next_free_addr + num_bytes + sizeof(block_header_t));

  if (total_needed_space > heap_end_addr) {
    void *new_end = kbrk(total_needed_space);
    if (new_end < total_needed_space) {
      return NULL;
    }
  }

  void *ptr = next_free_addr;
  block_header_t *header_ptr = (block_header_t *)ptr;
  header_ptr->size = num_bytes;
  header_ptr->magic = MAGIC;

  next_free_addr =
      (void *)((uint32_t)next_free_addr + num_bytes + sizeof(block_header_t));

  return (void *)((uint32_t)ptr + sizeof(block_header_t));
}
