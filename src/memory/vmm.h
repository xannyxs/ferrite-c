#ifndef VMM_H
#define VMM_H

#define PAGE_FLAG_PRESENT (1 << 0)
#define PAGE_FLAG_WRITABLE (1 << 1)
#define PAGE_FLAG_SUPERVISOR (1 << 2)

void vmm_init_pages(void);

#endif /* VMM_H */
