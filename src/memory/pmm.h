#ifndef PMM_H
#define PMM_H

#include "arch/x86/multiboot.h"

#include <stdint.h>

#define PAGE_SIZE 0x1000
#define NUM_PAGE_FRAMES (0x2000000 / PAGE_SIZE)

static uint8_t pmm_bitmap[NUM_PAGE_FRAMES / 8]; // Dynamic, bit array

extern uint32_t _physical_kernel_end;

static inline void pmm_clear_bit(uint32_t paddr) {
  uint32_t i = paddr / PAGE_SIZE;
  uint32_t byte = i / 8;
  uint8_t bit = i % 8;

  pmm_bitmap[byte] &= ~(1 << bit);
}

static inline void pmm_set_bit(uint32_t paddr) {
  uint32_t i = paddr / PAGE_SIZE;
  uint32_t byte = i / 8;
  uint8_t bit = i % 8;

  pmm_bitmap[byte] |= (1 << bit);
}

uint32_t pmm_alloc_page(void);

void pmm_init_from_map(multiboot_info_t *mbd);

#endif /* PMM_H */
