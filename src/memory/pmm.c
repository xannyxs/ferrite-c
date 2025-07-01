#include "memory/pmm.h"
#include "arch/x86/multiboot.h"
#include "drivers/printk.h"
#include "lib/math.h"
#include "string.h"

#include <stdint.h>

extern volatile uint8_t pmm_bitmap[];

void pmm_print_bit(uint32_t addr) {
  uint32_t i = addr / PAGE_SIZE;
  uint32_t byte_index = i / 8;
  uint8_t bit_index = i % 8;

  uint8_t data_byte = pmm_bitmap[byte_index];

  if ((data_byte >> bit_index) & 1) {
    printk("Bit at 0x%x is: 1\n", addr);
  } else {
    printk("Bit at 0x%x is: 0\n", addr);
  }
}

void pmm_print_bitmap() {
  printk("--- PMM Bitmap State ---\n");

  for (int32_t i = 0; i < 8 * 1024 * 1024 / PAGE_SIZE / 8; i++) {
    uint8_t byte = pmm_bitmap[i];

    for (int32_t j = 7; j >= 0; j--) {
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

  printk("\n------------------------\n");
}

uint32_t get_total_memory(multiboot_info_t *mbd) {
  uint32_t total_memory = 0;

  for (uint32_t i = 0; i < mbd->mmap_length; i += sizeof(multiboot_mmap_t)) {
    multiboot_mmap_t *entry = (multiboot_mmap_t *)(mbd->mmap_addr + i);
    if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
      uint32_t block_end = entry->addr_low + entry->len_low;
      if (block_end > total_memory) {
        total_memory = block_end;
      }
    }
  }

  return total_memory;
}

void pmm_init_from_map(multiboot_info_t *mbd) {
  uint32_t total_memory = get_total_memory(mbd);
  uint32_t pmm_bitmap_size = total_memory / PAGE_SIZE / 8;
  uint32_t bitmap_end_addr = (uint32_t)&pmm_bitmap + pmm_bitmap_size;

  uint32_t first_free_page_addr = ALIGN(bitmap_end_addr, PAGE_SIZE);

  uint32_t kernel_end = (uint32_t)&_kernel_end;
  printk("Kernel physical end: 0x%x\n", kernel_end);

  for (size_t i = 0; i < pmm_bitmap_size; i++) {
    pmm_bitmap[i] = 0xFF;
  }

  for (uint32_t i = 0; i < mbd->mmap_length; i += sizeof(multiboot_mmap_t)) {
    multiboot_mmap_t *entry = (multiboot_mmap_t *)(mbd->mmap_addr + i);

    if (entry->type != MULTIBOOT_MEMORY_AVAILABLE) {
      continue;
    }

    printk("Start Addr Low: 0x%x | Length Low: 0x%x\n", entry->addr_low,
           entry->len_low);

    for (uint32_t p = 0; p < entry->len_low; p += PAGE_SIZE) {
      uint32_t current_addr = entry->addr_low + p;

      if (current_addr >= first_free_page_addr) {
        pmm_clear_bit(current_addr);
      }
    }
  }

  pmm_print_bitmap();
}
