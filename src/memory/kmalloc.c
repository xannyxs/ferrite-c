#include "drivers/printk.h"
#include "lib/math.h"
#include "memory/block.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static uint32_t heap_start = KMALLOC_START;
static uint32_t heap_size = 0;
static uint32_t threshold = 0;

void change_heap_size(size_t new_size) {
  (void)threshold;
  (void)heap_start;

  int32_t old_page_top = CEIL_DIV(heap_size, 0x1000);
  int32_t new_page_top = CEIL_DIV(new_size, 0x1000);

  int32_t diff = new_page_top - old_page_top;

  for (int32_t i = 0; i < diff; i++) {
    uint32_t paddr = pmm_alloc_page_frame();
    mem_map_page(KMALLOC_START + old_page_top * 0x1000 + i * 0x1000, paddr,
                 PAGE_FLAG_WRITE);
  }
}

void init_kmalloc(size_t init_heap_size) { change_heap_size(init_heap_size); }

void *kmalloc(size_t size) {
  if (size == 0) {
    return NULL;
  }
  return NULL;
}
