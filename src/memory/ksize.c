#include "lib/stdlib.h"
#include "memory/consts.h"
#include "memory/kmalloc.h"

size_t ksize(void *ptr) {
  if (!ptr) {
    return 0;
  }

  block_header_t *header_ptr = (block_header_t *)ptr - 1;

  if (header_ptr->magic != MAGIC) {
    abort("Corrupt pointer provided to ksize");
  }

  size_t size_in_bytes = (1 << header_ptr->order) * PAGE_SIZE;
  return size_in_bytes;
}
