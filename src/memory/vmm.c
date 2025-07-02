#include "memory/vmm.h"
#include "lib/stdlib.h"
#include "memory/kmalloc.h"
#include "memory/pmm.h"
#include "string.h"

#include <stdint.h>

extern void flush_tbl();
extern void load_page_directory(uint32_t *);
extern void enable_paging();

uint32_t page_directory[1024] __attribute__((aligned(4096)));

uint32_t vmm_alloc_page_frame(void) {
  void *ptr = (void *)next_free_page;
  uint32_t addr_to_set = next_free_page;
  pmm_set_bit(addr_to_set);

  return (uint32_t)ptr;
}

void vmm_map_page(void *physaddr, void *virtualaddr, uint32_t flags) {
  uint32_t pdindex = (uint32_t)virtualaddr >> 22;
  uint32_t ptindex = (uint32_t)virtualaddr >> 12 & 0x03FF;

  uint32_t *pd = (uint32_t *)0xFFFFF000;
  uint32_t *pt = (uint32_t *)(0xFFC00000 + (pdindex * PAGE_SIZE));

  if (!(pd[pdindex] & PAGE_FLAG_PRESENT)) {
    uint32_t addr = vmm_alloc_page_frame();
    if (addr == 0) {
      abort("Out of physical memory");
    }

    pd[pdindex] =
        addr | PAGE_FLAG_SUPERVISOR | PAGE_FLAG_WRITABLE | PAGE_FLAG_PRESENT;
    flush_tbl();

    memset(&pt, 0, sizeof(pt));
  }

  uint32_t *pte = &pt[ptindex];
  if (*pte & PAGE_FLAG_PRESENT) {
    abort("Virtual Address is already taken");
  }

  pt[ptindex] = ((uint32_t)physaddr) | (flags & 0xFFF) | PAGE_FLAG_PRESENT;

  flush_tbl();
}

void vmm_init_pages(void) {
  for (int32_t i = 0; i < 1024; i++) {
    page_directory[i] = PAGE_FLAG_SUPERVISOR | PAGE_FLAG_WRITABLE;
  }

  uint32_t pt_phys_addr = vmm_alloc_page_frame();
  if (pt_phys_addr == 0) {
    abort("Out of physical memory");
  }

  uint32_t *pt = (uint32_t *)pt_phys_addr;
  for (uint32_t i = 0; i < 1024; i++) {
    uint32_t page_phys_addr = i * PAGE_SIZE;
    pt[i] = page_phys_addr | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITABLE |
            PAGE_FLAG_SUPERVISOR;
  }

  page_directory[0] = pt_phys_addr | PAGE_FLAG_SUPERVISOR | PAGE_FLAG_WRITABLE |
                      PAGE_FLAG_PRESENT;
  page_directory[1023] = (uint32_t)page_directory | PAGE_FLAG_PRESENT |
                         PAGE_FLAG_WRITABLE | PAGE_FLAG_SUPERVISOR;

  load_page_directory(page_directory);
  enable_paging();
}
