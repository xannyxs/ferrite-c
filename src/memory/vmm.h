#ifndef VMM_H
#define VMM_H

#include <stdint.h>

#define PTE_P (1 << 0)
#define PTE_W (1 << 1)
#define PTE_U (1 << 2)

#define ZONE_NORMAL 896 * 1024 * 1024

void *vmm_find_free_region(uint32_t);

void vmm_init_pages(void);

int32_t vmm_map_page(void *physaddr, void *virtualaddr, uint32_t flags);

void *vmm_unmap_page(void *);

void vmm_remap_page(void *vaddr, void *paddr, int32_t flags);

void *vmm_setup_process(void);

#endif /* VMM_H */
