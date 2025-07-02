#include "memory/kmalloc.h"
#include "lib/math.h"
#include "memory/pmm.h"

#include <stddef.h>
#include <stdint.h>

void *kmalloc(size_t num_bytes) {
  if (num_bytes == 0) {
    return NULL;
  }

  uint32_t pages_needed = CEIL_DIV(num_bytes, PAGE_SIZE);
  void *ptr = (void *)next_free_page;

  printk("Allocator: Attempting to allocate %d page(s) at 0x%x\n", pages_needed,
         next_free_page);

  // TODO: Double check if it is actually free
  for (uint32_t i = 0; i < pages_needed; i++) {
    uint32_t addr_to_set = next_free_page + (i * PAGE_SIZE);
    pmm_set_bit(addr_to_set);
  }

  next_free_page += pages_needed * PAGE_SIZE;

  return ptr;
}
