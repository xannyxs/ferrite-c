#ifndef PMM_H
#define PMM_H

#define PAGE_SIZE 0x1000

#include "arch/x86/multiboot.h"
#include "drivers/printk.h"

#include <stdint.h>

extern uint32_t _kernel_end;
extern volatile uint8_t pmm_bitmap[];

static inline void pmm_clear_bit(uint32_t addr) {
  if (addr == 0x110000) {
    printk("PMM Init: Clearing bit for 0x%x\n", addr);
  }

  uint32_t i = addr / PAGE_SIZE;
  uint32_t byte = i / 8;
  uint8_t bit = i % 8;

  pmm_bitmap[byte] &= ~(1 << bit);
}

static inline void pmm_set_bit(uint32_t addr) {
  if (addr == 0x110000) {
    printk("PMM Set: Setting bit for 0x%x\n", addr);
  }
  uint32_t i = addr / PAGE_SIZE;
  uint32_t byte = i / 8;
  uint8_t bit = i % 8;

  pmm_bitmap[byte] |= (1 << bit);
}

void pmm_init_from_map(multiboot_info_t *);

void pmm_print_bitmap(void);

void pmm_print_bit(uint32_t addr);

#endif /* PMM_H */
