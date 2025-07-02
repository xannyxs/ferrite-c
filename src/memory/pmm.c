#include "memory/pmm.h"
#include "arch/x86/multiboot.h"
#include "drivers/printk.h"
#include "lib/math.h"
#include "memory/vmm.h"
#include "string.h"

#include <stdint.h>

static uint32_t pmm_bitmap_size = 0;

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
  uint32_t i = 0;

  for (; i < pmm_bitmap_size; i++) {
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

  printk("Amount of bytes: %d\n", i);
}

uint32_t pmm_alloc_frame(void) {
  for (uint32_t i = 0; i < pmm_bitmap_size; i++) {
    uint32_t byte_index = i / 8;
    uint32_t bit_index = i % 8;

    if (pmm_bitmap[byte_index] & (1 << bit_index)) {
      continue;
    }

    uint32_t addr = i * PAGE_SIZE;
    pmm_set_bit(addr);
    return addr;
  }

  return 0;
}

void *pmm_get_physaddr(void *vaddr) {
  uint32_t pdindex = (uint32_t)vaddr >> 22;
  uint32_t ptindex = (uint32_t)vaddr >> 12 & 0x03FF;
  uint32_t offset = (uint32_t)vaddr & 0xFFF;
  uint32_t *pd = (uint32_t *)0xFFFFF000;

  if (!(pd[pdindex] & PAGE_FLAG_PRESENT)) {
    return 0;
  }

  uint32_t *pt = (uint32_t *)(0xFFC00000 + (pdindex * PAGE_SIZE));
  if (!(pt[ptindex] & PAGE_FLAG_PRESENT)) {
    return 0;
  }

  uint32_t phys_frame_addr = pt[ptindex] & ~0xFFF;

  return (void *)(phys_frame_addr + offset);
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
  pmm_bitmap_size = total_memory / PAGE_SIZE / 8;
  uint32_t bitmap_end_addr = (uint32_t)&pmm_bitmap + pmm_bitmap_size;

  uint32_t first_free_page_addr = ALIGN(bitmap_end_addr, PAGE_SIZE);

  uint32_t kernel_end = (uint32_t)&_kernel_end;
  printk("Kernel physical end: 0x%x\n", kernel_end);
  printk("first free page: 0x%x\n", first_free_page_addr);

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
