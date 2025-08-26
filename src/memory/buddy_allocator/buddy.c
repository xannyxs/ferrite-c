#include "memory/buddy_allocator/buddy.h"
#include "arch/x86/memlayout.h"
#include "debug/debug.h"
#include "drivers/printk.h"
#include "lib/math.h"
#include "lib/stdlib.h"
#include "memory/consts.h"
#include "memory/memblock.h"
#include "memory/vmm.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static buddy_allocator_t g_buddy = {0};

/* Private */

static int buddy_get_bit(int bit_index) {
  uint32_t byte_index = bit_index / 8;
  uint32_t bit_in_byte = bit_index % 8;

  if (byte_index >= g_buddy.map_size) {
    return 1;
  }

  return (g_buddy.map[byte_index] & (1 << bit_in_byte)) ? 1 : 0;
}

void buddy_visualize(void) {
  printk("\n--- Buddy Allocator Visualization ---\n");
  printk("  Base Address: 0x%x | Total Size: %u KB | Max Order: %u\n",
         (void *)g_buddy.base, g_buddy.size / 1024, g_buddy.max_order);
  printk("-------------------------------------\n");

  printk("  Free Lists by Order:\n");
  for (int i = 0; i <= g_buddy.max_order; i++) {
    buddy_node_t *node = g_buddy.free_lists[i];
    int free_block_count = 0;
    while (node) {
      free_block_count++;
      vmm_remap_page(SCRATCH_VADDR, node, 0);

      buddy_node_t *mapped_node = (buddy_node_t *)SCRATCH_VADDR;
      node = mapped_node->next;
    }
    int pages_per_block = 1 << i;
    printk("    Order %d (%d pages): %d free blocks\n", i, pages_per_block,
           free_block_count);
  }
  printk("-------------------------------------\n");

  printk("  Allocation Bitmap ('X' = allocated, '.' = free):\n");
  size_t total_pages = g_buddy.size / PAGE_SIZE;
  int bits_per_line = 32;

  for (size_t i = 0; i < total_pages; i++) {
    if (i % bits_per_line == 0) {
      if (i > 0) {
        printk("\n");
      }
      printk("    [%d-%d] ", i, i + bits_per_line - 1);
    }

    printk("%c", buddy_get_bit(i) ? 'X' : '.');
  }

  printk("\n-------------------------------------\n");
}

static bool is_block_free(uint32_t i, uint32_t order) {
  uint32_t blocks_to_check = 1 << order;

  for (uint32_t j = 0; j < blocks_to_check; j++) {
    uint32_t current_page_index = i + j;
    uint32_t byte_index = current_page_index / 8;
    uint32_t bit_in_byte = current_page_index % 8;

    if (byte_index >= g_buddy.map_size) {
      return false;
    }

    if ((g_buddy.map[byte_index] & (1 << bit_in_byte)) != 0) {
      return false;
    }
  }

  return true;
}

void remove_from_free_list(uintptr_t paddr, uint32_t order) {
  if (order > g_buddy.max_order) {
    abort("Invalid order provided to remove_from_free_list");
  }

  buddy_node_t *head = g_buddy.free_lists[order];

  if ((uintptr_t)head == paddr) {
    g_buddy.free_lists[order] = head->next;
    return;
  }

  buddy_node_t *prev = head;
  buddy_node_t *current = head->next;

  while (current) {
    if ((uintptr_t)current == paddr) {
      prev->next = current->next;
      return;
    }
    prev = current;
    current = current->next;
  }

  abort("Failed to remove address from free list: address not found!");
}

static void mark_free(uintptr_t paddr, uint32_t order) {
  uint32_t i = (paddr - g_buddy.base) / PAGE_SIZE;
  uint32_t blocks_to_mark = 1 << order;

  for (uint32_t j = 0; j < blocks_to_mark; j++) {
    uint32_t current = i + j;
    uint32_t byte_index = current / 8;
    uint32_t bit_in_byte = current % 8;

    if (byte_index >= g_buddy.map_size) {
      abort("Allocation address out of map bounds!");
    }

    g_buddy.map[byte_index] &= ~(1 << bit_in_byte);
  }
}

static void mark_allocated(uintptr_t paddr, uint32_t order) {
  uint32_t i = (paddr - g_buddy.base) / PAGE_SIZE;
  uint32_t blocks_to_mark = 1 << order;

  for (uint32_t j = 0; j < blocks_to_mark; j++) {
    uint32_t current = i + j;
    uint32_t byte_index = current / 8;
    uint32_t bit_in_byte = current % 8;

    if (byte_index >= g_buddy.map_size) {
      abort("Allocation address out of map bounds!");
    }

    g_buddy.map[byte_index] |= (1 << bit_in_byte);
  }
}

static void buddy_list_add(uintptr_t paddr, uint32_t k) {
  vmm_remap_page(SCRATCH_VADDR, (void *)paddr, 0);
  buddy_node_t *mapped_buddy = (buddy_node_t *)SCRATCH_VADDR;

  mapped_buddy->next = g_buddy.free_lists[k];
  g_buddy.free_lists[k] = (buddy_node_t *)paddr;
}

/* Public */

size_t buddy_get_total_memory(void) { return g_buddy.size; }

void buddy_dealloc(uintptr_t addr, uint32_t order) {
  mark_free(addr, order);

  uintptr_t current_addr = addr;
  uint32_t current_order = order;

  while (current_order < g_buddy.max_order) {
    size_t block_size = PAGE_SIZE << current_order;
    uintptr_t buddy_addr = current_addr ^ block_size;

    if (!is_block_free(buddy_addr, current_order)) {
      break;
    }

    remove_from_free_list(buddy_addr, current_order);

    current_addr = (current_addr < buddy_addr) ? current_addr : buddy_addr;
    current_order += 1;
  }

  buddy_list_add(current_addr, current_order);
}

void *buddy_alloc(uint32_t order) {
  if (order > g_buddy.max_order) {
    return NULL;
  }

  uint32_t k = order;
  while (k < g_buddy.max_order && !g_buddy.free_lists[k]) {
    k += 1;
  }

  if (!g_buddy.free_lists[k]) {
    printk("Out of memory. No suitable block found.\n");
    return NULL;
  }

  buddy_node_t *node = g_buddy.free_lists[k];
  uintptr_t block_addr = (uintptr_t)node;

  vmm_remap_page(SCRATCH_VADDR, (void *)node, 0);
  buddy_node_t *mapped_node = (buddy_node_t *)SCRATCH_VADDR;
  g_buddy.free_lists[k] = mapped_node->next;

  while (k > order) {
    k -= 1;
    uintptr_t buddy_addr = block_addr + (PAGE_SIZE << k);

    buddy_list_add(buddy_addr, k);
  }

  mark_allocated(block_addr, order);
  return (void *)block_addr;
}

void buddy_init(void) {
  uintptr_t start_addr = (uintptr_t)get_next_free_addr();
  uintptr_t end_addr = (uintptr_t)get_heap_end_addr();

  size_t num_pages = (end_addr - start_addr) / PAGE_SIZE;
  size_t map_size_needed = CEIL_DIV(num_pages, 8);
  g_buddy.map_size = ALIGN(map_size_needed, sizeof(size_t));

  g_buddy.map = (uint8_t *)memblock(g_buddy.map_size);
  if (!g_buddy.map) {
    abort("Could not allocate the bitmap for Buddy Allocator");
  }
  memset(g_buddy.map, 0, g_buddy.map_size);

  g_buddy.base = ALIGN((uintptr_t)g_buddy.map + g_buddy.map_size, PAGE_SIZE);
  g_buddy.size = end_addr - g_buddy.base;
  if (g_buddy.size < PAGE_SIZE) {
    abort("Not enough memory for the buddy pool");
  }

  g_buddy.max_order = floor_log2(g_buddy.size / PAGE_SIZE);

  for (int32_t i = 0; i <= MAX_ORDER; i++) {
    g_buddy.free_lists[i] = NULL;
  }

  size_t remaining = g_buddy.size;
  uintptr_t current_addr = g_buddy.base;

  while (remaining >= PAGE_SIZE) {
    uint32_t order = floor_log2(remaining / PAGE_SIZE);
    if (order > g_buddy.max_order) {
      order = g_buddy.max_order;
    }

    vmm_remap_page(SCRATCH_VADDR, (void *)current_addr, 0);
    buddy_node_t *block = (buddy_node_t *)SCRATCH_VADDR;

    block->next = g_buddy.free_lists[order];
    g_buddy.free_lists[order] = (buddy_node_t *)current_addr;

    size_t block_size = PAGE_SIZE << order;
    current_addr += block_size;
    remaining -= block_size;
  }
}
