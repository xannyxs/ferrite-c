#include "lib/stdlib.h"
#include "memory/memory.h"

size_t ksize(void *ptr) {
  if (!ptr) {
    return 0;
  }

  block_header_t *header_ptr = (block_header_t *)ptr - 1;

  if (header_ptr->magic != MAGIC) {
    abort("Corrupt pointer provided to ksize");
  }

  return header_ptr->size;
}
