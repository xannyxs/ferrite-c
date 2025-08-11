#include "memory/vmm.h"
#include "arch/x86/memlayout.h"
#include "drivers/printk.h"
#include "lib/stdlib.h"
#include "memory/consts.h"
#include "memory/pmm.h"
#include "string.h"

#include <stdint.h>

/* i386 does not support invld. Using flush_tlb() instead
 * https://wiki.osdev.org/TLB
 */
extern void flush_tlb(void);
extern void load_page_directory(uint32_t *);
extern void enable_paging(void);

uint32_t page_directory[1024] __attribute__((aligned(4096)));

/* Public */

void *vmm_unmap_page(void *vaddr) {
  printk("Vaddr: 0x%x\n", (uint32_t)vaddr);

  uint32_t pdindex = (uint32_t)vaddr >> 22;
  uint32_t ptindex = (uint32_t)vaddr >> 12 & 0x03FF;

  uint32_t *pt = (uint32_t *)(0xFFC00000 + (pdindex * PAGE_SIZE));
  uint32_t *pte = &pt[ptindex];

  if (!(*pte & PAGE_FLAG_PRESENT)) {
    return NULL;
  }

  void *paddr = (void *)(*pte & ~0xFFF);
  *pte = 0;
  flush_tlb();

  return paddr;
}

void vmm_map_page(void *physaddr, void *virtualaddr, uint32_t flags) {
  uint32_t pdindex = (uint32_t)virtualaddr >> 22;
  uint32_t ptindex = (uint32_t)virtualaddr >> 12 & 0x03FF;

  uint32_t *pd = (uint32_t *)0xFFFFF000;
  uint32_t *pt = (uint32_t *)(0xFFC00000 + (pdindex * PAGE_SIZE));

  if (!(pd[pdindex] & PAGE_FLAG_PRESENT)) {
    uint32_t paddr = pmm_alloc_frame();
    if (paddr == 0) {
      abort("Out of physical memory");
    }

    pd[pdindex] =
        paddr | PAGE_FLAG_SUPERVISOR | PAGE_FLAG_WRITABLE | PAGE_FLAG_PRESENT;
    flush_tlb();

    memset(pt, 0, PAGE_SIZE);
  }

  if (pt[ptindex] & PAGE_FLAG_PRESENT) {
    abort("Virtual Address is already taken");
  }

  pt[ptindex] = ((uint32_t)physaddr) | (flags & 0xFFF) | PAGE_FLAG_PRESENT;

  flush_tlb();
}

void vmm_init_pages(void) {
  for (int32_t i = 0; i < 1024; i++) {
    page_directory[i] = PAGE_FLAG_SUPERVISOR | PAGE_FLAG_WRITABLE;
  }

  uintptr_t pt_phys_addr = pmm_alloc_frame();
  if (pt_phys_addr == 0) {
    abort("Out of physical memory when allocating initial page table");
  }

  uint32_t pt_virt_addr = P2V_WO(pt_phys_addr);
  uint32_t *pt = (uint32_t *)pt_virt_addr;
  for (uint32_t i = 0; i < 1024; i++) {
    uint32_t page_phys_addr = i * PAGE_SIZE;
    pt[i] = page_phys_addr | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITABLE |
            PAGE_FLAG_SUPERVISOR;
  }

  uint32_t page_directory_paddr = V2P_WO((uint32_t)page_directory);
  page_directory[0] = pt_phys_addr | PAGE_FLAG_SUPERVISOR | PAGE_FLAG_WRITABLE |
                      PAGE_FLAG_PRESENT;
  page_directory[768] = pt_phys_addr | PAGE_FLAG_SUPERVISOR |
                        PAGE_FLAG_WRITABLE | PAGE_FLAG_PRESENT;
  page_directory[1023] = page_directory_paddr | PAGE_FLAG_PRESENT |
                         PAGE_FLAG_WRITABLE | PAGE_FLAG_SUPERVISOR;

  load_page_directory((uint32_t *)page_directory_paddr);
  enable_paging();
}
