#include "pmm.h"
#include "arch/x86/multiboot.h"
#include "drivers/printk.h"
#include "lib/math.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static uint32_t page_frame_min;
static uint32_t page_frame_max;
static uint32_t total_alloc;

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

void pmm_init_from_map(multiboot_info_t *mbd) {
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

  pmm_print_bitmap();
  printk("PMM bitmap initialized.\n");
}

uint32_t pmm_alloc_page_frame(void) {
  uint32_t start = page_frame_min / 8 + ((page_frame_min & 7) != 0 ? 1 : 0);
  uint32_t end = page_frame_max / 8 - ((page_frame_max & 7) != 0 ? 1 : 0);

  for (uint32_t b = start; b < end; b++) {
    uint8_t byte = pmm_bitmap[b];
    if (byte == 0xFF) {
      continue;
    }

    for (uint32_t i = 0; i < 8; i++) {
      bool used = byte >> i & 1;

      if (!used) {
        byte ^= (-1 ^ byte) & (1 << i);
        pmm_bitmap[b] = byte;
        total_alloc = total_alloc + 1;

        uint32_t paddr = (b * 8 + i) * PAGE_SIZE;

        return paddr;
      }
    }
  }

  // TODO: Give proper error handling
  return 0;
}

void pmm_init(uint32_t mem_low, uint32_t mem_upper) {
  page_frame_min = CEIL_DIV(mem_low, PAGE_SIZE);
  page_frame_max = mem_upper / PAGE_SIZE;
  total_alloc = 0;

  memset(pmm_bitmap, 0, sizeof(pmm_bitmap));
}
