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

  block_header_t *header = (block_header_t *)ptr - 1;
  if (header->magic != MAGIC) {
    printk("Ptr is corrupt\n");
    return;
  }

  uintptr_t block_vaddr = (uintptr_t)header;
  size_t block_size = header->size;

  free_list_t *freed_block = (free_list_t *)block_vaddr;
  freed_block->size = block_size;

  free_list_t *current = get_free_list_head();
  free_list_t *prev = NULL;

  while (current && (uintptr_t)current < (uintptr_t)freed_block) {
    prev = current;
    current = current->next;
  }

  if (prev && (uintptr_t)prev + prev->size == (uintptr_t)freed_block) {
    prev->size += freed_block->size;
    freed_block = prev;
  } else {
    if (prev) {
      prev->next = freed_block;
    } else {
      set_free_list_head(freed_block);
    }
  }

  if (current &&
      (uintptr_t)freed_block + freed_block->size == (uintptr_t)current) {
    freed_block->size += current->size;
    freed_block->next = current->next;
  } else {
    freed_block->next = current;
  }

  for (size_t i = 1; i < block_size / PAGE_SIZE; i++) {
    uintptr_t current_vaddr = block_vaddr + i * PAGE_SIZE;
    void *paddr = vmm_unmap_page((void *)current_vaddr);
    if (paddr) {
      buddy_dealloc((uintptr_t)paddr, 0);
    }
  }
}
