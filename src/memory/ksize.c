#include "lib/stdlib.h"
#include "memory/memblock.h"

size_t ksize(void *ptr) {
  if (!ptr) {
    return 0;
  }

  block_header_t *header_ptr = (block_header_t *)ptr - 1;

  if (header_ptr->magic != MAGIC) {
    abort("Corrupt pointer");
  }

  return header_ptr->size;
}
