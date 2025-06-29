#include "memory/vmm.h"
#include "debug/debug.h"
#include "drivers/printk.h"
#include "lib/math.h"
#include "lib/stdlib.h"
#include "memory/block.h"
#include "memory/pmm.h"

#include <stddef.h>
#include <stdint.h>

void vmm_expand_heap(size_t new_size) {
  (void)kernel_heap_top;
  (void)initial_page_dir;

  if (new_size <= kernel_heap_mapped_size) {
    return;
  }

  int32_t old_page_top = CEIL_DIV(kernel_heap_mapped_size, PAGE_SIZE);
  int32_t new_page_top = CEIL_DIV(new_size, PAGE_SIZE);

  int32_t diff = new_page_top - old_page_top;
  if (diff <= 0) {
    return;
  }

  for (int32_t i = 0; i < diff; i++) {
    uint32_t paddr = pmm_alloc_page();
    if (paddr == 0) {
      abort("Out of physical memory during heap expansion!");
    }

    bochs_print_string("after pmm alloc page\n");
    BOCHS_MAGICBREAK();

    uint32_t vaddr = VMM_START + (old_page_top + i) * PAGE_SIZE;
    mem_map_page(vaddr, paddr, PAGE_FLAG_WRITE | PAGE_FLAG_PRESENT);

    bochs_print_string("after map page\n");
    BOCHS_MAGICBREAK();
  }

  kernel_heap_mapped_size = new_page_top * PAGE_SIZE;
}
