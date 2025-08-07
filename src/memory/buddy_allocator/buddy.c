#include "memory/buddy_allocator/buddy.h"
#include "drivers/printk.h"
#include "lib/math.h"
#include "lib/stdlib.h"
#include "memory/consts.h"
#include "memory/kmalloc.h"
#include "memory/memblock.h"

#include <stddef.h>
#include <stdint.h>

static buddy_allocator_t g_buddy = {0};

/* Private */

void buddy_print_state() {
  printk("\n--- Buddy Allocator State ---\n");
  printk("  Base Address:   0x%x\n", (void *)g_buddy.base);
  printk("  Total Size:     %u KB\n", g_buddy.size / 1024);
  printk("  Max Order:      %u\n", g_buddy.max_order);
  printk("  Bitmap Address:   0x%x\n", g_buddy.map);
  printk("  --- Free Lists by Order ---\n");

  for (int i = 0; i <= g_buddy.max_order; i++) {
    buddy_node_t *node = g_buddy.free_lists[i];
    int free_block_count = 0;

    while (node != NULL) {
      free_block_count++;
      node = node->next;
    }

    int pages_per_block = 1 << i;

    printk("    Order %d (%d pages): %d free blocks\n", i, pages_per_block,
           free_block_count);
  }
  printk("-----------------------------\n");
}

/* Public */

void buddy_init(uintptr_t base, size_t given_size) {
  g_buddy.base = base;
  g_buddy.size = given_size;
  int32_t block_count = given_size / PAGE_SIZE;

  if (block_count == 0) {
    abort("No blocks!");
  }

  g_buddy.max_order = 0;
  int32_t tmp = block_count;
  while (tmp > 1) {
    tmp >>= 1;
    g_buddy.max_order += 1;
  }

  int32_t managed_blocks = 1 << g_buddy.max_order;
  int32_t bitmap_bytes = CEIL_DIV(managed_blocks, 8);
  int32_t bitmap_size = CEIL_DIV(bitmap_bytes, sizeof(size_t)) * sizeof(size_t);

  g_buddy.map = (uint8_t *)g_buddy.base;
  uintptr_t allocatable_base = ALIGN(g_buddy.base + bitmap_size, PAGE_SIZE);
  buddy_node_t *initial_block = (buddy_node_t *)allocatable_base;
  initial_block->next = NULL;
  g_buddy.free_lists[g_buddy.max_order] = initial_block;

  for (int i = 0; i < g_buddy.max_order; i++) {
    g_buddy.free_lists[i] = NULL;
  }

  buddy_print_state();

  set_allocator(BUDDY);
}
