#include "arch/x86/memlayout.h"
#include "drivers/printk.h"
#include "lib/stdlib.h"
#include "memory/consts.h"
#include "memory/memblock.h"
#include "memory/vmm.h"

#include <stdint.h>

void kfree(void *ptr) {
  if (!ptr) {
    printk("ptr is null\n");
    return;
  }
  block_header_t *header_ptr = (block_header_t *)ptr - 1;
  printk("Size: %d \n", header_ptr->size);

  if (header_ptr->magic != MAGIC) {
    abort("Corrupt pointer");
  }

  for (uint32_t i = 0; i < header_ptr->size / PAGE_SIZE; i++) {
    vmm_unmap_page((void *)((uint32_t)ptr + i * PAGE_SIZE));
  }
}
