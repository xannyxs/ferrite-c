#ifndef VMM_H
#define VMM_H

#include <stdint.h>

#define PAGE_FLAG_PRESENT (1 << 0)
#define PAGE_FLAG_WRITABLE (1 << 1)
#define PAGE_FLAG_SUPERVISOR (1 << 2)

void *vmm_find_free_region(uint32_t);

void vmm_map_page(void *physaddr, void *virtualaddr, uint32_t flags);

void vmm_init_pages(void);

void vmm_unmap_page(void *vaddr);

#endif /* VMM_H */
