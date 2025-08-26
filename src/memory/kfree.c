#include "arch/x86/memlayout.h"
#include "drivers/printk.h"
#include "lib/math.h"
#include "lib/stdlib.h"
#include "memory/buddy_allocator/buddy.h"
#include "memory/consts.h"
#include "memory/kmalloc.h"
#include "memory/memory.h"

#include <stdint.h>

void kfree(void *ptr) {
  if (!ptr) {
    printk("ptr is null\n");
    return;
  }

  block_header_t *header = (block_header_t *)ptr - 1;
  if (header->magic != MAGIC) {
    abort("The given pointer is corrupt\n");
  }

  if ((header->flags & MEM_ALLOCATOR_MASK) != MEM_TYPE_KMALLOC) {
    abort("kfree() is used on a vmalloc() pointer\n");
  }

  uintptr_t vaddr = (uintptr_t)header;
  size_t size = header->size;
  uint32_t pages = CEIL_DIV(size, PAGE_SIZE);
  uint32_t order = ceil_log2(pages);

  uintptr_t paddr = V2P_WO(vaddr);
  buddy_dealloc(paddr, order);
}
