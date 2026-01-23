#ifndef VMM_H
#define VMM_H

#include <uapi/types.h>

#define PTE_P (1 << 0)
#define PTE_W (1 << 1)
#define PTE_U (1 << 2)

#define ZONE_NORMAL 896 * 1024 * 1024

void vmm_init_pages(void);

s32 vmm_map_page(void* paddr, void* vaddr, u32 flags);

void* vmm_unmap_page(void*);

void vmm_remap_page(void* vaddr, void* paddr, s32 flags);

void vmm_free_pagedir(void* pgdir);

void vmm_clear_pages(void);

#endif /* VMM_H */
