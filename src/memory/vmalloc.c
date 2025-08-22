#include "memory/vmalloc.h"
#include "drivers/printk.h"
#include "lib/math.h"
#include "lib/stdlib.h"
#include "memory/buddy_allocator/buddy.h"
#include "memory/consts.h"
#include "memory/memory.h"
#include "memory/vmm.h"

#include <stddef.h>
#include <stdint.h>

static free_list_t *virtual_free_list_head = NULL;

/* Public */

free_list_t *get_free_list_head(void) { return virtual_free_list_head; }

void set_free_list_head(free_list_t *list) { virtual_free_list_head = list; }

void vmalloc_init(void) {
  uintptr_t heap_start_addr = (uintptr_t)HEAP_START;

  void *first_page_phys = buddy_alloc(0);
  if (!first_page_phys) {
    abort("Not enough physical memory to start the heap!");
  }

  int32_t ret = vmm_map_page(first_page_phys, (void *)heap_start_addr, 0);
  if (ret < 0) {
    printk("Should not be possible\n");
  }

  size_t max_virtual_size = 0xFFFFFFFF - heap_start_addr;
  size_t total_physical_memory = buddy_get_total_memory();

  size_t heap_size = max_virtual_size;
  if (total_physical_memory < heap_size) {
    heap_size = total_physical_memory;
  }

  if (heap_size < sizeof(free_list_t)) {
    abort("Not enough memory to allocate a free_list");
  }

  virtual_free_list_head = (free_list_t *)heap_start_addr;
  virtual_free_list_head->size = heap_size;
  virtual_free_list_head->next = NULL;
}

/**
 * @brief Allocates a virtually contiguous memory block.
 *
 * Slower than kmalloc. Used to allocate large memory regions that do not need
 * to be physically contiguous, such as for kernel modules.
 *
 * @param size The number of bytes to allocate.
 * @return A pointer to the allocated memory, or NULL on failure.
 */
void *vmalloc(size_t num_bytes) {
  if (num_bytes == 0 || !virtual_free_list_head) {
    return NULL;
  }

  size_t total_size = ALIGN(num_bytes + sizeof(block_header_t), PAGE_SIZE);

  free_list_t *current = virtual_free_list_head;
  free_list_t *prev = NULL;
  while (current) {
    if (current->size >= total_size) {
      break;
    }
    prev = current;
    current = current->next;
  }

  if (!current) {
    printk("Not enough Memory");
    return NULL;
  }

  uintptr_t vaddr = (uintptr_t)current;
  if (current->size - total_size > sizeof(free_list_t)) {
    free_list_t *new_free_node = (free_list_t *)(vaddr + total_size);

    void *header_phys_page = buddy_alloc(0);
    if (!header_phys_page) {
      abort("Out of physical memory while splitting block!");
    }

    int32_t ret = vmm_map_page(header_phys_page, (void *)new_free_node, 0);
    if (ret < 0) {
      printk("Page already exists\n");
    }

    new_free_node->size = current->size - total_size;
    new_free_node->next = current->next;

    if (!prev) {
      virtual_free_list_head = new_free_node;
    } else {
      prev->next = new_free_node;
    }
  } else {
    if (!prev) {
      virtual_free_list_head = current->next;
    } else {
      prev->next = current->next;
    }
  }

  for (size_t i = 0; i < total_size / PAGE_SIZE; i += 1) {
    void *phys_page = buddy_alloc(0);
    if (!phys_page) {
      abort("Out of physical memory during mapping");
    }
    vmm_map_page(phys_page, (void *)(vaddr + i * PAGE_SIZE), 0);
  }

  block_header_t *header = (block_header_t *)vaddr;
  header->magic = MAGIC;
  header->size = total_size;

  return (void *)(header + 1);
}
