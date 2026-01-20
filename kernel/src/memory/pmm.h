#ifndef PMM_H
#define PMM_H

#include "arch/x86/multiboot.h"
#include "memory/consts.h"
#include <types.h>

extern u32 _kernel_end[];
extern u8 volatile pmm_bitmap[];

static inline void pmm_clear_bit(u32 addr)
{
    u32 i = addr / PAGE_SIZE;
    u32 byte = i >> 3; // Equivalent to i / 8
    u8 bit = i & 0x7;  // Equivalent to i % 8

    pmm_bitmap[byte] &= ~(1 << bit);
}

void* pmm_get_physaddr(void* vaddr);

void pmm_init_from_map(multiboot_info_t*);

void pmm_print_bitmap(void);

void pmm_print_bit(u32 addr);

void pmm_free_frame(void* paddr);

u32 pmm_get_first_addr(void);

u32 pmm_bitmap_len(void);

#endif /* PMM_H */
