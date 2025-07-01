#include "memory/kmalloc.h"
#include "debug/debug.h"
#include "lib/math.h"
#include "memory/pmm.h"

#include <stddef.h>
#include <stdint.h>

static uint32_t next_free_page = 0x110000;

void *kmalloc(size_t num_bytes) {
  if (num_bytes == 0) {
    return NULL;
  }

  uint32_t pages_needed = CEIL_DIV(num_bytes, PAGE_SIZE);
  void *ptr = (void *)next_free_page;

  printk("Allocator: Attempting to allocate %d page(s) at 0x%x\n", pages_needed,
         next_free_page);

  for (uint32_t i = 0; i < pages_needed; i++) {
    uint32_t addr_to_set = next_free_page + (i * PAGE_SIZE);
    pmm_print_bit(addr_to_set + i);
    pmm_set_bit(addr_to_set);
    pmm_print_bit(addr_to_set + i);
  }

  next_free_page += pages_needed * PAGE_SIZE;

  return ptr;
}
