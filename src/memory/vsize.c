#include "lib/stdlib.h"
#include "memory/memory.h"

void vbrk(void) {
  abort("cannot find a valid break. We are using buddy_allocator");
}

size_t vsize(void *ptr) {
  if (!ptr) {
    return 0;
  }

  block_header_t *header_ptr = (block_header_t *)ptr - 1;

  if (header_ptr->magic != MAGIC) {
    abort("Corrupt pointer provided to ksize");
  }

  return header_ptr->size;
}
