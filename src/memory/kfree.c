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
    abort("Corrupt pointer provided to ksize");
  }

  buddy_dealloc((uintptr_t)header_ptr, header_ptr->order);

  // for (uint32_t i = 0; i < header_ptr->size / PAGE_SIZE; i++) {
  //   vmm_unmap_page((void *)((uint32_t)ptr + i * PAGE_SIZE));
  // }
}
