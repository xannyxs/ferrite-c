#include "arch/x86/memlayout.h"
#include "drivers/printk.h"
#include "memory/kmalloc.h"
#include "memory/pmm.h"
#include "memory/vmm.h"

#include <stdint.h>

void kfree(void *ptr) {
  if (!ptr) {
    printk("ptr is null\n");
    return;
  }
  block_header_t *header_ptr = (block_header_t *)ptr - 1;
  printk("Size: %d | Magic: 0x%x\n", header_ptr->size, header_ptr->magic);

  for (uint32_t i = 0; i < header_ptr->size / PAGE_SIZE; i++) {
    vmm_unmap_page(ptr + i * PAGE_SIZE);
  }
}
