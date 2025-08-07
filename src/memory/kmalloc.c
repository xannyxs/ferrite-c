#include "memory/kmalloc.h"
#include "memory/memblock.h"

#include <stddef.h>

static enum alloc_e g_allocator = NONE;

/* Public */

enum alloc_e get_allocator(void) { return g_allocator; }

void set_allocator(enum alloc_e new_alloc) { g_allocator = new_alloc; }

void *kmalloc(size_t n) {
  switch (g_allocator) {
  case NONE:
    return NULL;
  case MEMBLOCK:
    return memblock(n);
  case BUDDY:
    return NULL;
  case SLAB:
    return NULL;
  }

  __builtin_unreachable();
}
