#include "memory/pmm.h"
#include "arch/x86/multiboot.h"
#include "drivers/printk.h"
#include "string.h"

#include <stdint.h>

void pmm_print_bitmap() {
  printk("--- PMM Bitmap State ---\n");

  for (int32_t i = 0; i < 8 * 1024 * 1024 / PAGE_SIZE / 8; i++) {
    uint8_t byte = pmm_bitmap[i];
    for (int32_t j = 7; j > 0; j--) {
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

void pmm_init_from_map(multiboot_info_t *mbd) {
  uint32_t kernel_end = (uint32_t)&_kernel_end;
  printk("Kernel physical end: 0x%x\n", kernel_end);

  memset(pmm_bitmap, 0xFF, sizeof(pmm_bitmap));

  for (uint32_t i = 0; i < mbd->mmap_length; i += sizeof(multiboot_mmap_t)) {
    multiboot_mmap_t *entry = (multiboot_mmap_t *)(mbd->mmap_addr + i);

    if (entry->type != MULTIBOOT_MEMORY_AVAILABLE) {
      continue;
    }

    printk("Start Addr Low: 0x%x | Length Low: 0x%x\n", entry->addr_low,
           entry->len_low);

    for (uint32_t p = 0; p < entry->len_low; p += PAGE_SIZE) {
      uint32_t current_addr = entry->addr_low + p;

      if (current_addr >= kernel_end) {
        pmm_clear_bit(current_addr);
      }
    }
  }

  pmm_print_bitmap();
}
