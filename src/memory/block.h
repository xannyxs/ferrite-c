#ifndef BLOCK_H
#define BLOCK_H

#include "arch/x86/multiboot.h"

#include <stdint.h>

#define KMALLOC_START 0xD0000000

#define PAGE_FLAG_PRESENT (1 << 0)
#define PAGE_FLAG_WRITE (1 << 1)
#define PAGE_FLAG_OWNER (1 << 9)

#define REC_PAGEDIR ((uint32_t *)0xFFFFF000)
#define REC_PAGETABLE(i) ((uint32_t *)0xFFC00000) + ((i) << 12)

extern uint32_t KERNEL_VIRTUAL_BASE;
extern uint32_t initial_page_dir[1024];

void mem_map_page(uint32_t vaddr, uint32_t paddr, uint32_t flags);

void init_memory(uint32_t, uint32_t);

#endif /* BLOCK_H */
