#include "pmm.h"
#include "arch/x86/multiboot.h"
#include "drivers/printk.h"
#include "lib/math.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static uint32_t page_frame_min;
static uint32_t page_frame_max;

/* Private */

static void pmm_print_bitmap() {
  printk("--- PMM Bitmap State ---\n");
  for (int i = 0; i < 256; i++) {
    uint8_t byte = pmm_bitmap[i];
    for (int j = 7; j >= 0; j--) {
      if ((byte >> j) & 1) {
        printk("1");
      } else {
        printk("0");
      }
    }
    printk(" ");
    if ((i + 1) % 16 == 0) {
      printk("\n");
    }
  }
  printk("------------------------\n");
}

/* Public */

uint32_t pmm_alloc_page(void) {

  for (uint32_t i = page_frame_min; i < page_frame_max; i++) {
    uint32_t byte_index = i / 8;
    uint8_t bit_in_byte = i % 8;

    if (!((pmm_bitmap[i] >> bit_in_byte) & 1)) {
      pmm_bitmap[byte_index] |= (1 << bit_in_byte);

      return i * PAGE_SIZE;
    }
  }

  printk("PMM Error: Out of physical memory!\n");
  return 0;
}

void pmm_init_from_map(multiboot_info_t *mbd) {
  uint32_t highest_addr = 0;

  printk("Processing Multiboot memory map...\n");
  for (uint32_t i = 0; i < mbd->mmap_length;
       i += sizeof(multiboot_memory_map_t)) {
    multiboot_memory_map_t *mmmt =
        (multiboot_memory_map_t *)(mbd->mmap_addr + i);

    if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) {
      uint32_t block_end_addr = mmmt->addr_low + mmmt->len_low;

      if (block_end_addr > highest_addr) {
        highest_addr = block_end_addr;
      }
    }
  }

  page_frame_max = highest_addr / PAGE_SIZE;

  uint32_t kernel_physical_end = (uint32_t)&_physical_kernel_end;
  printk("Kernel physical end: 0x%x\n", kernel_physical_end);

  memset(pmm_bitmap, 0xFF, sizeof(pmm_bitmap));

  for (uint32_t i = 0; i < mbd->mmap_length;
       i += sizeof(multiboot_memory_map_t)) {
    multiboot_memory_map_t *mmmt =
        (multiboot_memory_map_t *)(mbd->mmap_addr + i);

    if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) {
      printk("Start Addr Low: 0x%x | Length Low: 0x%x\n", mmmt->addr_low,
             mmmt->len_low);

      for (uint32_t j = 0; j < mmmt->len_low; j += PAGE_SIZE) {
        uint32_t current_page_addr = mmmt->addr_low + j;

        if (current_page_addr >= kernel_physical_end) {
          pmm_clear_bit(current_page_addr);
        }
      }
    }
  }

  uint32_t first_safe_addr = kernel_physical_end + sizeof(pmm_bitmap);
  page_frame_min = CEIL_DIV(first_safe_addr, PAGE_SIZE);

  pmm_print_bitmap();
  printk("PMM bitmap initialized.\n");
  printk("  Max Page Frame (Total System Pages): %d\n", page_frame_max);
  printk("  Min Page Frame (First Available): %d\n", page_frame_min);
  printk("  PMM is ready to allocate pages between 0x%x and 0x%x.\n",
         page_frame_min * PAGE_SIZE, page_frame_max * PAGE_SIZE);
}
