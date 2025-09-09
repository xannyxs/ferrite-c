#ifndef VMM_H
#define VMM_H

#include <stdbool.h>
#include <stdint.h>

#define PTE_P (1 << 0)
#define PTE_W (1 << 1)
#define PTE_U (1 << 2)

#define ZONE_NORMAL 896 * 1024 * 1024

void *vmm_find_free_region(uint32_t);

void vmm_init_pages(void);

int32_t vmm_map_page(void *physaddr, void *virtualaddr, uint32_t flags);

int32_t vmm_map_page_dir(void *pdir, void *paddr, void *vaddr, uint32_t flags);

void *vmm_unmap_page(void *);

void vmm_remap_page(void *vaddr, void *paddr, int32_t flags);

uint32_t *vmm_setup_pgdir(void);

uint32_t *vmm_walkpgdir(uint32_t *pgdir, const void *vaddr, bool alloc);

void print_mapping_info(uint32_t vaddr);

#endif /* VMM_H */
