#include "memory/kmalloc.h"

#include <stddef.h>

static void *heap_start_addr = NULL;
static void *next_free_addr = NULL;

void kmalloc_init() {
  kmem_init();
  heap_start_addr = HEAP_START;
  next_free_addr = heap_start_addr;
}

void *kmalloc(size_t num_bytes) {
  if (num_bytes == 0) {
    return NULL;
  }

  void *heap_end_addr = get_current_break();
  void *total_needed_space =
      next_free_addr + num_bytes + sizeof(block_header_t);

  if (total_needed_space > heap_end_addr) {
    void *new_end = kbrk(total_needed_space);
    if (new_end < total_needed_space) {
      return NULL;
    }
  }

  void *ptr = next_free_addr;
  block_header_t *header_ptr = (block_header_t *)ptr;
  header_ptr->size = num_bytes;
  header_ptr->magic = 0xDEADBEEF;

  next_free_addr += num_bytes + sizeof(block_header_t);

  return ptr + sizeof(block_header_t);
}
