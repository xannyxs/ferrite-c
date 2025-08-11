#include "arch/x86/memlayout.h"
#include "drivers/printk.h"
#include "lib/stdlib.h"
#include "memory/buddy_allocator/buddy.h"
#include "memory/consts.h"
#include "memory/kmalloc.h"
#include "memory/vmm.h"

#include <stdint.h>

void kfree(void *ptr) {
  if (!ptr) {
    printk("ptr is null\n");
    return;
  }

  block_header_t *header_ptr = (block_header_t *)ptr - 1;
  if (header_ptr->magic != MAGIC) {
    abort("Corrupt pointer provided");
  }

  uintptr_t raw_block_vaddr = (uintptr_t)header_ptr;
  int32_t order = header_ptr->order;
  size_t allocation_size = (1 << order) * PAGE_SIZE;

  uintptr_t first_page_paddr = 0;

  for (size_t i = 0; i < allocation_size; i += PAGE_SIZE) {
    uintptr_t paddr = (uintptr_t)vmm_unmap_page((void *)(raw_block_vaddr + i));

    if (i == 0) {
      first_page_paddr = paddr;
    }
  }

  if (first_page_paddr == 0) {
    abort("Failed to get physical address during unmap!");
  }

  buddy_dealloc(first_page_paddr, order);
}
